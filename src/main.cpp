#define _WIN32_WINNT 0x0A00
#define NTDDI_VERSION NTDDI_WIN10_CO
#include <sdkddkver.h>
#if !defined(_WIN32)
    #error "This library is only supported on Windows."
#endif

#if NTDDI_VERSION < NTDDI_WIN10_CO
    #error "This library is only supported on Windows 11 Build 22000 or later."
#endif

#include "../include/lunalify/lunalify.hpp"
#include "../include/lunalify/events.hpp"
#include "windows.h"
#include "winnt.h"
#include <shellapi.h>
#include <fstream>
#include <string>
#include <filesystem>
#include <winrt/base.h>

bool g_isDaemonProcess = false;
HINSTANCE g_hInstDLL = NULL;
static HANDLE s_hEventPipe = INVALID_HANDLE_VALUE;

namespace fs = std::filesystem;


std::wstring to_wstring(const char* str) {
    if (!str) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);
    std::wstring wstr(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str, -1, &wstr[0], len);
    // Removes the null terminator extra
    if (!wstr.empty() && wstr.back() == L'\0') {
        wstr.pop_back();
    }
    return wstr;
}

std::wstring get_current_exe_path() {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    return std::wstring(buffer);
}

static ToastConfig fill_base_config(lua_State* L) {
    // LNF_WARN("Stack", "Idx 1: {} | Idx 2: {} | Idx 3: {}",
    //     luaL_optstring(L, 1, "nil"),
    //     luaL_optstring(L, 2, "nil"),
    //     luaL_optstring(L, 3, "nil"));
    ToastConfig config;
    config.title    = luaL_checkstring(L, 1);
    config.message  = luaL_checkstring(L, 2);
    config.appId    = luaL_checkstring(L, 3);
    config.iconPath = luaL_optstring(L, 4, "");
    config.heroPath = luaL_optstring(L, 5, "");
    config.sound    = luaL_optstring(L, 6, "Default");
    config.silent   = lua_toboolean(L, 7);
    config.duration = luaL_optstring(L, 8, "short");
    config.scenario = luaL_optstring(L, 9, "default");

    config.tag      = luaL_optstring(L, 13, "");
    config.group    = luaL_optstring(L, 14, "");
    return config;
}


static int l_register(lua_State* L) {
    const char* appId = luaL_checkstring(L, 1);
    const char* appName = luaL_checkstring(L, 2);
    const char* iconPath= luaL_optstring(L, 3, "");
    const char* daemonLogPath = luaL_optstring(L, 4, "");
    const char* exePath = "";


    std::wstring wAppId = to_wstring(appId);
    std::wstring wAppName = to_wstring(appName);

    auto code = register_app_identity(wAppId, wAppName);
    LNF_INFO("App:Register", "Registering app {} ({})", appName, appId);
    if (code != Lunalify::Errors::Code::SUCCESS) {
        lua_pushboolean(L, false);
        lua_pushstring(L, Lunalify::Errors::GetMessage(code).c_str());
        lua_pushinteger(L, (int)code);
        return 3;
    }


    wchar_t* appdata = _wgetenv(L"APPDATA");
    if (!appdata) {
        code = Lunalify::Errors::Code::SYS_PATH_NOT_FOUND;
        lua_pushboolean(L, false);
        lua_pushstring(L, Lunalify::Errors::GetMessage(code).c_str());
        lua_pushinteger(L, (int)code);
        return 3;
    }
    std::wstring wExePath = exePath[0] ? to_wstring(exePath) : get_current_exe_path();
    std::wstring wIconPath= to_wstring(iconPath);
    std::wstring shortcutPath = std::wstring(appdata) +
        L"\\Microsoft\\Windows\\Start Menu\\Programs\\" + wAppName + L".lnk";
    LNF_INFO("App:Register", "Creating shortcut {}", winrt::to_string(shortcutPath));

    code = create_app_shortcut(wAppId, wExePath, shortcutPath, wIconPath);
    if (code != Lunalify::Errors::Code::SUCCESS) {
        lua_pushboolean(L, false);
        lua_pushstring(L, Lunalify::Errors::GetMessage(code).c_str());
        lua_pushinteger(L, (int)code);
        return 3;
    }
    LNF_INFO("App:Register", "Shortcut created");
    DWORD currentPid = GetCurrentProcessId();
    std::wstring eventName = Lunalify::Globals::globalLunalifyReadyEvent(currentPid);
    HANDLE hEvent = CreateEventW(NULL, TRUE, FALSE, eventName.c_str());


    // Starts daemon (if not already running)
    code = Lunalify::Daemon::Spawn(daemonLogPath, currentPid);
    if (code != Lunalify::Errors::Code::SUCCESS) {
        lua_pushboolean(L, false);
        lua_pushstring(L, Lunalify::Errors::GetMessage(code).c_str());
        lua_pushinteger(L, (int)code);
        return 3;
    }
    // if (GetLastError() == ERROR_ALREADY_EXISTS) {
    //     lua_pushboolean(L, true);
    //     return 1;
    // }

    lua_pushboolean(L, true);
    // Wait for daemon initialization event (15s timeout for safety)
    DWORD waitResult = WaitForSingleObject(hEvent, 15000);
    if (waitResult == WAIT_OBJECT_0) {
        LNF_WARN("App:Register", "Daemon initialization completed");
    } else {
        LNF_WARN("App:Register", "Daemon initialization timed out");
        return 1;
    }
    CloseHandle(hEvent);
    return 1;
}

static int l_unregister(lua_State* L) {
    const char* appId = luaL_checkstring(L, 1);
    const char* appName = luaL_checkstring(L, 2);

    std::wstring wAppId = to_wstring(appId);
    std::wstring wAppName = to_wstring(appName);

    wchar_t* appdata = _wgetenv(L"APPDATA");
    if (!appdata) {
        lua_pushboolean(L, false);
        return 1;
    }

    std::wstring shortcutPath = std::wstring(appdata) +
        L"\\Microsoft\\Windows\\Start Menu\\Programs\\" + wAppName + L".lnk";

    LNF_INFO("App:Unregister", "Unregistering app {} ({})", appName, appId);

    auto code = unregister_app_identity(wAppId, shortcutPath);

    if (code == Lunalify::Errors::Code::SUCCESS) {
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, false);
        lua_pushstring(L, Lunalify::Errors::GetMessage(code).c_str());
        return 2;
    }

    return 1;
}


static int l_wait_event(lua_State* L) {
    // HANDLE hPipe = INVALID_HANDLE_VALUE;
    const wchar_t* pipeName = Lunalify::Protocol::EVENT_PIPE_NAME;

    if (s_hEventPipe == INVALID_HANDLE_VALUE) {
        int attempts = 100;
        // bool ready = false;

        while (attempts > 0) {
            s_hEventPipe = CreateFileW(
                pipeName,
                GENERIC_READ,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
            );
            if (s_hEventPipe != INVALID_HANDLE_VALUE) {
                DWORD mode = PIPE_READMODE_MESSAGE;
                SetNamedPipeHandleState(s_hEventPipe, &mode, NULL, NULL);
                break;
            }
            DWORD err = GetLastError();
            if (err == ERROR_PIPE_BUSY) {
                WaitNamedPipeW(pipeName, 1000);
            } else {
                Sleep(50);
            }
            attempts--;
        }
    }

    if (s_hEventPipe == INVALID_HANDLE_VALUE) {
        LNF_ERROR("App:Bridge:WaitEvent", "Could not connect to Daemon Event Pipe.");
        return 0;
    }

    char buffer[8192];
    DWORD bytesRead;
    LNF_INFO("App:Bridge:WaitEvent", "Reading from Pipe");
    BOOL success = ReadFile(s_hEventPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
    if (!success || bytesRead == 0) {
        LNF_WARN("App:Bridge:WaitEvent", "Connection lost. Resetting handle.");
        CloseHandle(s_hEventPipe);
        s_hEventPipe = INVALID_HANDLE_VALUE;
        return 0;
    }
    // CloseHandle(s_hEventPipe);
    buffer[bytesRead] = '\0';
    std::string raw(buffer);
    std::vector<std::string> parts;
    size_t start = 0, end = 0;
    while ((end = raw.find(Lunalify::Protocol::SEPARATOR, start)) != std::string::npos) {
        parts.push_back(raw.substr(start, end - start));
        start = end + 1;
    }
    parts.push_back(raw.substr(start));

    if (parts.size() >= 3) {
        lua_newtable(L);
        lua_pushstring(L, parts[1].c_str()); lua_setfield(L, -2, "id");
        lua_pushstring(L, parts[0].c_str()); lua_setfield(L, -2, "event");
        lua_pushstring(L, parts[2].c_str()); lua_setfield(L, -2, "args");
        LNF_DEBUG("Bridge", "Received event {}", parts[0].c_str());

        return 1;
    }
    return 0;
}


static int l_send_toast(lua_State* L) {
    ToastConfig config = fill_base_config(L);

    // int top = lua_gettop(L);
    if (lua_istable(L, 10)) {
        lua_pushnil(L);
        while (lua_next(L, 10) != 0) {
            ToastAction action;
            lua_getfield(L, -1, "label");
            action.label = luaL_optstring(L, -1, "OK");
            lua_pop(L, 1);

            lua_getfield(L, -1, "target");
            action.target = luaL_optstring(L, -1, "");
            lua_pop(L, 1);

            lua_getfield(L, -1, "placement");
            action.placement = luaL_optstring(L, -1, "default");
            lua_pop(L, 1);

            lua_getfield(L, -1, "image");
            action.image = luaL_optstring(L, -1, "");
            lua_pop(L, 1);

            lua_getfield(L, -1, "type");
            action.type = luaL_optstring(L, -1, "protocol");
            lua_pop(L, 1);

            lua_getfield(L, -1, "style");
            action.style = luaL_optstring(L, -1, "");
            // printf("Action: %s | Style: %s\n", action.label.c_str(), action.style.c_str());
            lua_pop(L, 1);

            config.actions.push_back(action);
            lua_pop(L, 1);
        }
    }

    if (lua_istable(L, 11)) {
            lua_pushnil(L);
            while (lua_next(L, 11) != 0) {
                ToastInput input;
                lua_getfield(L, -1, "id");
                input.id = luaL_checkstring(L, -1); lua_pop(L, 1);
                lua_getfield(L, -1, "type");
                input.type = luaL_checkstring(L, -1); lua_pop(L, 1);
                lua_getfield(L, -1, "placeholder");
                input.placeholder = luaL_optstring(L, -1, ""); lua_pop(L, 1);
                lua_getfield(L, -1, "title");
                input.title = luaL_optstring(L, -1, "");
                lua_pop(L, 1);
                lua_getfield(L, -1, "defaultInput");
                input.defaultInput = luaL_optstring(L, -1, ""); lua_pop(L, 1);

                lua_getfield(L, -1, "selections");
                if (lua_istable(L, -1)) {
                    lua_pushnil(L);
                    while (lua_next(L, -2) != 0) {
                        ToastSelection sel;
                        lua_getfield(L, -1, "id");
                        sel.id = luaL_checkstring(L, -1); lua_pop(L, 1);
                        lua_getfield(L, -1, "content");
                        sel.content = luaL_checkstring(L, -1); lua_pop(L, 1);
                        input.selections.push_back(sel);
                        lua_pop(L, 1);
                    }
                }
                lua_pop(L, 1);

                config.inputs.push_back(input);
                lua_pop(L, 1);
            }
        }

    if (lua_istable(L, 12)) {
        lua_getfield(L, 12, "active");
        config.progress.active = lua_toboolean(L, -1); lua_pop(L, 1);

        lua_getfield(L, 12, "title");
        config.progress.title = luaL_optstring(L, -1, ""); lua_pop(L, 1);

        lua_getfield(L, 12, "value");
        config.progress.value = luaL_optstring(L, -1, "0"); lua_pop(L, 1);

        lua_getfield(L, 12, "displayValue");
        config.progress.displayValue = luaL_optstring(L, -1, ""); lua_pop(L, 1);

        lua_getfield(L, 12, "status");
        config.progress.status = luaL_optstring(L, -1, ""); lua_pop(L, 1);
    }

    auto GenerateRandomId = []() {
        return "ntf_" + std::to_string(GetTickCount64());
    };


    std::string uniqueId = config.tag.empty() ? GenerateRandomId() : config.tag;
    Lunalify::Errors::Code result = send_toast(config, uniqueId);

    if (result == Lunalify::Errors::Code::SUCCESS) {
        lua_pushstring(L, uniqueId.c_str());
        return 1; // Returns id (success)
        } else {
            lua_pushnil(L); // arg 1: Id = nil
            lua_pushstring(L, Lunalify::Errors::GetMessage(result).c_str()); // arg 2: Mgs
            lua_pushinteger(L, (int)result); // arg 3: Error code
        return 3;
    }
}

static int l_update_toast(lua_State* L) {
    if (lua_gettop(L) < 6) {
            LNF_ERROR("Bridge", "update_toast expected 6 args, got {}", lua_gettop(L));
            lua_pushboolean(L, false);
            return 1;
        }
    std::string appId = luaL_checkstring(L, 1);
    std::string tag = luaL_checkstring(L, 2);
    std::string title = luaL_checkstring(L, 3);
    std::string value = luaL_checkstring(L, 4);
    std::string display = luaL_optstring(L, 5, "");
    std::string status = luaL_optstring(L, 6, "");
    LNF_DEBUG("Bridge:Pipe", "Sending Update Command for Tag: {}", tag);

    std::string payload = Lunalify::Protocol::CMD_UPDATE_TOAST + Lunalify::Protocol::SEPARATOR +
                              appId + Lunalify::Protocol::SEPARATOR +
                              tag + Lunalify::Protocol::SEPARATOR +
                              title + Lunalify::Protocol::SEPARATOR +
                              value + Lunalify::Protocol::SEPARATOR +
                              display + Lunalify::Protocol::SEPARATOR +
                              status;

    HANDLE hPipe = CreateFileW(Lunalify::Protocol::CMD_PIPE_NAME, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hPipe != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteFile(hPipe, payload.c_str(), (DWORD)payload.size(), &written, NULL);
        CloseHandle(hPipe);
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

static int l_set_logger(lua_State* L) {
    bool enabled = lua_toboolean(L, 1);
    int level = (int)luaL_optinteger(L, 2, 1);
    const char* path = luaL_optstring(L, 3, "");
    bool console_out = lua_isboolean(L, 4) ? lua_toboolean(L, 4) : true;

    Lunalify::Logger::Initialize(enabled, (Lunalify::LogLevel)level, path, console_out);
    return 0;
}

static int l_shutdown(lua_State* L) {
    HANDLE hPipe = CreateFileW(
        Lunalify::Protocol::CMD_PIPE_NAME,
        GENERIC_WRITE,
        0, NULL,
        OPEN_EXISTING,
        0, NULL
    );

    if (hPipe != INVALID_HANDLE_VALUE) {
        std::string cmd = Lunalify::Protocol::CMD_EXIT + Lunalify::Protocol::SEPARATOR;

        DWORD written;
        BOOL success = WriteFile(hPipe, cmd.c_str(), (DWORD)cmd.size(), &written, NULL);
        CloseHandle(hPipe);
        lua_pushboolean(L, success);
        LNF_INFO("Bridge", "Shutdown command sent to Daemon.");
    } else {
        LNF_WARN("Bridge", "Could not connect to Daemon for shutdown. Error: {}", GetLastError());
        lua_pushboolean(L, false);
    }
    return 1;
}

// static int l_poll_event(lua_State* L) {
//     HANDLE hPipe = CreateFileW(Lunalify::Protocol::EVENT_PIPE_NAME,  GENERIC_READ,
//     FILE_SHARE_READ | FILE_SHARE_WRITE, // allows shared read/write
//     NULL,
//     OPEN_EXISTING,
//     0,
//     NULL
//     );

//     if (hPipe == INVALID_HANDLE_VALUE) {
//         lua_pushnil(L);
//         return 1;
//     }

//     DWORD bytesAvail = 0;
//     if (PeekNamedPipe(hPipe, NULL, 0, NULL, &bytesAvail, NULL) && bytesAvail > 0) {
//         LNF_INFO("Bridge", "Event pipe has data, reading...");
//         char buffer[8192];
//         DWORD read;
//         if (ReadFile(hPipe, buffer, sizeof(buffer)-1, &read, NULL)) {
//             buffer[read] = '\0';
//             LNF_INFO("Bridge", "Event read: {}", buffer);
//             PushEventToLua(L, std::string(buffer));
//             CloseHandle(hPipe);
//             return 1;
//         }
//     }

//     CloseHandle(hPipe);
//     lua_pushnil(L);
//     return 1;
// }


extern "C" __declspec(dllexport) int luaopen_lunalify_core(lua_State* L) {
    // If the process is a daemon, execute and terminate here
    if (g_isDaemonProcess) {
        LPWSTR* szArglist;
        int nArgs;
        szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
        std::string daemonLogPath = "";
        DWORD ppid = {0};
        bool enableLog = false;

        if (szArglist) {
            for (int i = 0; i < nArgs; i++) {
                std::wstring arg = szArglist[i];
                if (arg == L"--daemon-log" && (i + 1) < nArgs) {
                    std::wstring val = szArglist[i + 1];
                    daemonLogPath = std::string(val.begin(), val.end());
                    enableLog = true;
                }
                if (arg == L"--ppid" && (i + 1) < nArgs) {
                    ppid = std::wcstoul(szArglist[i + 1], nullptr, 10);
                }
            }
         LocalFree(szArglist);
        }

        Lunalify::Logger::Initialize(enableLog, Lunalify::LogLevel::LEVEL_DEBUG, daemonLogPath, false);
        LNF_INFO("App:Daemon", "Daemon started. Parent PID: {}", ppid);
        Lunalify::Daemon::Run(ppid);
        LNF_ERROR("App:Daemon", "Process terminated unexpectedly.");
        ExitProcess(0);
    }
    static const struct luaL_Reg lunalify_lib[] = {
        {"register",   l_register},
        {"unregister", l_unregister},
        {"toast",      l_send_toast},
        {"update_toast", l_update_toast},
        {"wait_event", l_wait_event},
        {"set_logger", l_set_logger},
        {"shutdown",   l_shutdown},
        // {"poll_event", l_poll_event},
        {NULL, NULL}
    };
    luaL_newlib(L, lunalify_lib);
    return 1;
}


// Enables the DLL to intercept the process if it is the Daemon
extern "C" BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID /*lpReserved*/) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        g_hInstDLL = hModule; // Saves the DLL handle
        // If the process has the flag, it becomes a daemon
        if (wcsstr(GetCommandLineW(), L"--lunalify-daemon")) {
            g_isDaemonProcess = true;
        }
    }
    return TRUE;
}
