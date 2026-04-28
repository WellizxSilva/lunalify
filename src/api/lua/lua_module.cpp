#include "lunalify/api/types.hpp"
#include "lunalify/errors.hpp"
#include "lunalify/logger.hpp"
#include "lunalify/lunalify.hpp"
#include "lunalify/api/api_context.hpp"
#include "lunalify/api/lua/lua_mapper.hpp"
#include "lunalify/app/identity.hpp"
#include "lunalify/runtime/orchestrator.hpp"
#include "lunalify/runtime/notifications/notifier.hpp"
#include "lunalify/runtime/handlers/handle_daemon.hpp"
#include "winnt.h"

using namespace Lunalify::App;
using namespace Lunalify::Utils;
using namespace Lunalify::App::Lua::DataStructs;

namespace Lunalify::API::Lua::Invokers {
    static int l_register(lua_State* L) {
        RegisterData data;
        Lunalify::API::Lua::Mapper::FillRegister(L, 1, data, false);
        LNF_INFO("Lunalify:Invokers:Register", "Registering app: {}", data.appName);
        auto code = Identity::RegisterAppIdentity(to_wstring(data.appId), to_wstring(data.appName));
        if (code != Lunalify::Errors::Code::SUCCESS) {
            lua_pushboolean(L, false);
            lua_pushstring(L, Lunalify::Errors::TranslateCode(code).c_str());
            lua_pushinteger(L, (int)code);
            return 3;
        }
        wchar_t* appdata = _wgetenv(L"APPDATA");
        if (!appdata) {
            lua_pushboolean(L, false);
            lua_pushstring(L, "APPDATA environment variable not set");
            return 2;
        }
        std::wstring wShortcutPath = std::wstring(appdata) +
            L"\\Microsoft\\Windows\\Start Menu\\Programs\\" + to_wstring(data.appName) + L".lnk";

        code = Identity::CreateAppShortcut(to_wstring(data.appId), Lunalify::Utils::get_current_exe_path(), wShortcutPath, to_wstring(data.iconPath));
        if (code != Lunalify::Errors::Code::SUCCESS) {
            lua_pushboolean(L, false);
            lua_pushstring(L, Lunalify::Errors::TranslateCode(code).c_str());
            return 3;
        }
        DWORD currentPid = GetCurrentProcessId();
        std::wstring eventName = Lunalify::Globals::GlobalLunalifyReadyEvent(currentPid);
        HANDLE hEvent = CreateEventW(NULL, TRUE, FALSE, eventName.c_str());

        std::string daemonLogPath = data.daemonLogPath;
        LNF_INFO("Lunalify:Invokers:Register", "Daemon log path: {}", daemonLogPath);
        code = Lunalify::Runtime::Orchestrator::Spawn(daemonLogPath, currentPid);
        if (code != Lunalify::Errors::Code::SUCCESS) {
            CloseHandle(hEvent);
            lua_pushboolean(L, false);
            lua_pushstring(L, Lunalify::Errors::TranslateCode(code).c_str());
            return 2;
        }
        LNF_INFO("Lunalify:Invokers:Register", "Waiting for Orchestrator to initialize pipes...");
        DWORD waitResult = WaitForSingleObject(hEvent, 10000); // 10s timeout
        CloseHandle(hEvent);

        if (waitResult == WAIT_OBJECT_0) {
            LNF_INFO("Lunalify:Invokers:Register", "Orchestrator ready.");
            lua_pushboolean(L, true);
            return 1;
        } else {
            LNF_WARN("Lunalify:Invokers:Register", "Timeout or failure waiting for Orchestrator.");
            lua_pushboolean(L, false);
            lua_pushstring(L, "Orchestrator initialization timeout");
            return 2;
        }
    }

    static int l_unregister(lua_State* L) {
        RegisterData data;
        Lunalify::API::Lua::Mapper::FillRegister(L, 1, data, true);
        wchar_t* appdata = _wgetenv(L"APPDATA");
        if (!appdata) {
            lua_pushboolean(L, false);
            return 1;
        }
        std::wstring wShortcutPath = std::wstring(appdata) +
            L"\\Microsoft\\Windows\\Start Menu\\Programs\\" + to_wstring(data.appName) + L".lnk";

        LNF_INFO("Lunalify:Invokers:Unregister", "Removing shortcut from Start Menu: {}", data.appName);

        auto code = Identity::UnregisterAppIdentity(to_wstring(data.appId), wShortcutPath);
        lua_pushboolean(L, code == Lunalify::Errors::Code::SUCCESS);
        if (code != Lunalify::Errors::Code::SUCCESS) {
            lua_pushstring(L, Lunalify::Errors::TranslateCode(code).c_str());
            return 2;
        }
        return 1;
    }
    static int l_send_toast(lua_State* L) {
        ToastConfig config = Lunalify::API::Lua::Mapper::MapStackToToastConfig(L);
        std::string uniqueId = config.tag.empty() ?
            "ntf_" + std::to_string(GetTickCount64()) : config.tag;
        // Build the XML on client-side (avoid daemon overhead)
        std::wstring wXml = Lunalify::Runtime::Notifications::Notifier::BuildToastXml(config);
        std::string xml(wXml.begin(), wXml.end());

        std::string payload = config.appId + Lunalify::Protocol::SEPARATOR +
        uniqueId + Lunalify::Protocol::SEPARATOR +
        config.progress.title + Lunalify::Protocol::SEPARATOR +
        config.progress.value + Lunalify::Protocol::SEPARATOR +
        config.progress.displayValue + Lunalify::Protocol::SEPARATOR +
        config.progress.status + Lunalify::Protocol::SEPARATOR +
        xml;
        auto packet = Lunalify::Protocol::Pack(Lunalify::Protocol::OpCode::CMD_FIRE_TOAST, payload);
        auto& ctx = Lunalify::API::Context::Get();
        HANDLE hPipe = ctx.GetCmdPipe();
        DWORD written;
        BOOL success = FALSE;
        if (hPipe != INVALID_HANDLE_VALUE) {
            success = WriteFile(hPipe, packet.data(), (DWORD)packet.size(), &written, NULL);
            if (!success) {
                ctx.ResetCmdPipe();
                hPipe = INVALID_HANDLE_VALUE;
            }
        }

        if (hPipe == INVALID_HANDLE_VALUE) {
            // if invalid context, try open a new pipe
            hPipe = CreateFileW(Lunalify::Protocol::CMD_PIPE_NAME,
                     GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
            if (hPipe == INVALID_HANDLE_VALUE) {
                LNF_ERROR("Lunalify:Invokers:Toast", "Failed to open command pipe: {}", GetLastError());
                lua_pushnil(L);
                lua_pushstring(L, "Error trying to open command pipe (Daemon not found)");
                return 2;
            }
            ctx.SetCmdPipe(hPipe);
            success = WriteFile(hPipe, packet.data(), (DWORD)packet.size(), &written, NULL);
        }
        // DWORD written;
        // BOOL success = WriteFile(hPipe, packet.data(), (DWORD)packet.size(), &written, NULL);
        // FlushFileBuffers(hPipe);
        LNF_DEBUG("Lunalify:Invokers:Toast", "Sending packet. Total size: {} bytes. Header size: {}", packet.size(), sizeof(Lunalify::Protocol::PacketHeader));

        if (!success) {
            LNF_WARN("Lunalify:Invokers:Toast", "Pipe connection lost.");
            ctx.ResetCmdPipe();
            lua_pushnil(L);
            return 1;
        }
        FlushFileBuffers(hPipe);

        uint8_t ack = 0;
        DWORD read = 0;
        LNF_DEBUG("Lunalify:Invokers:Toast", "Waiting for ACK...");
        if (ReadFile(hPipe, &ack, 1, &read, NULL) && read > 0) {
            LNF_DEBUG("Lunalify:Invokers:Toast", "ACK received from Daemon");
        } else {
            // if the daemon close after 'ACK', 'ReadFile' can fail silently
            // if that happens, reset the pipe, to allow next call open a new one
            ctx.ResetCmdPipe();
        }
        // do not close the pipe here (avoid race conditions)
        // CloseHandle(hPipe);
        lua_pushstring(L, uniqueId.c_str());
        return 1;
    }

    static int l_wait_event(lua_State* L) {
        auto& ctx = Lunalify::API::Context::Get();
        HANDLE hPipe = ctx.GetEventPipe();

        if (hPipe == INVALID_HANDLE_VALUE) {
            hPipe = CreateFileW(
                Lunalify::Protocol::EVENT_PIPE_NAME,
                GENERIC_READ,
                0, NULL, OPEN_EXISTING, 0, NULL
            );

            if (hPipe == INVALID_HANDLE_VALUE) {
                lua_pushnil(L);
                return 1;
            }
            ctx.SetEventPipe(hPipe);
        }

        Lunalify::Protocol::PacketHeader header;
        DWORD bytesRead = 0;
        BOOL success = ReadFile(hPipe, &header, sizeof(header), &bytesRead, NULL);

        if (!success || bytesRead == 0) {
            // Lost connection, reset for next attempt
            ctx.ResetEventPipe();
            lua_pushnil(L);
            return 1;
        }

        if (header.magic != Lunalify::Protocol::MAGIC_HEADER) {
            LNF_ERROR("Lunalify:Invokers:WaitEvent", "Protocol mismatch. Magic: 0x{:X}", header.magic);
            ctx.ResetEventPipe();
            lua_pushnil(L);
            return 1;
        }

        std::string payload;
        if (header.payloadSize > 0) {
            payload.resize(header.payloadSize);
            ReadFile(hPipe, payload.data(), (DWORD)header.payloadSize, &bytesRead, NULL);
        }

        std::vector<std::string> parts;
        std::stringstream ss(payload);
        std::string item;
        while (std::getline(ss, item, Lunalify::Protocol::SEPARATOR)) {
            parts.push_back(item);
        }

        if (parts.size() >= 3) {
            lua_newtable(L);
            lua_pushstring(L, parts[0].c_str()); lua_setfield(L, -2, "event");
            lua_pushstring(L, parts[1].c_str()); lua_setfield(L, -2, "id");
            lua_pushstring(L, parts[2].c_str()); lua_setfield(L, -2, "args");
            return 1;
        }

        lua_pushnil(L);
        return 1;
    }

    static int l_update_toast(lua_State* L) {
        ToastUpdate data;
        Mapper::FillUpdate(L, 1, data);

        std::string payload = data.appId + Lunalify::Protocol::SEPARATOR +
                            data.tag + Lunalify::Protocol::SEPARATOR +
                            data.title + Lunalify::Protocol::SEPARATOR +
                            data.value + Lunalify::Protocol::SEPARATOR +
                            data.vString + Lunalify::Protocol::SEPARATOR +
                            data.status;
        LNF_DEBUG("Lunalify:Invokers:UpdateToast", "Sending Payload: [{}]", payload);

        auto packet = Lunalify::Protocol::Pack(Lunalify::Protocol::OpCode::CMD_UPDATE_TOAST, payload);
        auto& ctx = Lunalify::API::Context::Get();
        HANDLE hPipe = ctx.GetCmdPipe();
        if (hPipe == INVALID_HANDLE_VALUE) {
            hPipe = CreateFileW(Lunalify::Protocol::CMD_PIPE_NAME, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
            if (hPipe != INVALID_HANDLE_VALUE) {
                ctx.SetCmdPipe(hPipe);
            }
        }
        if (hPipe != INVALID_HANDLE_VALUE) {
            DWORD written;
            BOOL success = WriteFile(hPipe, packet.data(), (DWORD)packet.size(), &written, NULL);
            FlushFileBuffers(hPipe);
            if (success) {
                uint8_t ack = 0;
                DWORD read;
                ReadFile(hPipe, &ack, 1, &read, NULL);
                lua_pushboolean(L, true);
                LNF_DEBUG("Lunalify:Invokers:UpdateToast", "Sent payload successfully.");
            } else {
                ctx.ResetCmdPipe();
                lua_pushboolean(L, false);
            }
        } else {
            LNF_ERROR("Lunalify:Invokers:UpdateToast", "Failed to open CMD pipe: {}", GetLastError());
            lua_pushboolean(L, false);
        }
        return 1;
    }
    static int l_set_logger(lua_State* L) {
        LoggerData data;
        Mapper::FillLogger(L, 1, data);
        Lunalify::Logger::Initialize(data.enabled, data.level, data.path, data.to_console);
        return 0;
    }
    static int l_shutdown(lua_State* L) {
        auto& ctx = Lunalify::API::Context::Get();
        auto packet = Lunalify::Protocol::Pack(Lunalify::Protocol::OpCode::CMD_SHUTDOWN, "");
        HANDLE hPipe = ctx.GetCmdPipe();

        if (hPipe == INVALID_HANDLE_VALUE) {
            hPipe = CreateFileW(Lunalify::Protocol::CMD_PIPE_NAME, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        }

        if (hPipe == INVALID_HANDLE_VALUE) {
            // if cannot connect to pipe, probably daemon is not running
            lua_pushboolean(L, false);
            return 1;
        }
        DWORD written;
        BOOL success = WriteFile(hPipe, packet.data(), (DWORD)packet.size(), &written, NULL);
        if (success) {
            FlushFileBuffers(hPipe);
            LNF_INFO("Lunalify:Invokers:Shutdown", "Shutdown signal sent to Daemon.");
        }
        ctx.ResetCmdPipe();
        ctx.ResetEventPipe();
        lua_pushboolean(L, success);
        return 1;
    }
}

extern "C" __declspec(dllexport) int luaopen_lunalify_core(lua_State* L) {
    using namespace Lunalify::API::Lua::Invokers;
    if (wcsstr(GetCommandLineW(), L"--lunalify-daemon")) {
        auto& ctx =Lunalify::API::Context::Get();
        ctx.SetIsDaemon(true);
        Lunalify::Runtime::Handlers::HandleDaemonMode();
    }

    static const struct luaL_Reg lunalify_lib[] = {
        {"register",   l_register},
        {"unregister", l_unregister},
        {"toast",      l_send_toast},
        {"update_toast", l_update_toast},
        {"wait_event", l_wait_event},
        {"set_logger", l_set_logger},
        {"shutdown",   l_shutdown},
        {NULL, NULL}
    };

    luaL_newlib(L, lunalify_lib);
    return 1;
}
