#include "lunalify/logger.hpp"
#include "lunalify/lunalify.hpp"
#include "lunalify/runtime/orchestrator.hpp"
#include "lunalify/protocol.hpp"
#include "lunalify/runtime/notifications/notifier.hpp"
#include "lunalify/api/api_context.hpp"
#include "windows.h"
#include <winrt/Windows.Foundation.h>
#include <iostream>
#include <sstream>
#include <filesystem>

using namespace Lunalify::Utils;

static HANDLE g_hOrchestratorJob = NULL; // Job handle that tracks the orchestrator process

namespace Lunalify::Runtime {

    Orchestrator::Orchestrator() : m_running(false) {}

    Orchestrator::~Orchestrator() {
        Stop();
    }

    Errors::Code Orchestrator::Spawn(const std::string& logPath, DWORD parentPid) {
        HANDLE hMutex = CreateMutexW(NULL, TRUE, Lunalify::Locals::LocalLunalifyOrchestratorMutex().c_str());
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            if (hMutex) CloseHandle(hMutex);
            LNF_INFO("Lunalify:Orchestrator:Spawn", "An instance is already running.");
            return Errors::Code::SUCCESS;
        }

        if (g_hOrchestratorJob == NULL) {
            g_hOrchestratorJob = CreateJobObjectW(NULL, NULL);
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = { };
            jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            SetInformationJobObject(g_hOrchestratorJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));
        }

        auto& ctx = Lunalify::API::Context::Get();
        wchar_t dllPath[MAX_PATH];
        if (GetModuleFileNameW((HMODULE)ctx.GetDllInstance(), dllPath, MAX_PATH) == 0) {
            return Errors::Code::SYS_PATH_NOT_FOUND;
        }
        std::filesystem::path p(dllPath);
        std::filesystem::path binDir = p.parent_path();
        std::filesystem::path lunalifyDir = binDir.parent_path();
        std::filesystem::path projectRootDir = lunalifyDir.parent_path();
        if (projectRootDir.empty()) {
            projectRootDir = std::filesystem::current_path();
        }

        std::wstring logArg = L"";
        LNF_WARN("Lunalify:Orchestrator:Spawn", "Log path: {}", logPath);
        if (!logPath.empty()) {
            logArg = L" --daemon-log \"" + to_wstring(logPath.c_str()) + L"\"";
        }
        std::wstring exePath = Lunalify::Utils::get_current_exe_path();

        std::wstring cmdLine = L"\"" + exePath + L"\"" +
            L" -e \"package.path='.;./?.lua;./?/init.lua;'..package.path\"" +
            L" -l lunalify -- --lunalify-daemon" +
            logArg +
            L" --ppid " + std::to_wstring(parentPid);
        LNF_INFO("Lunalify:Orchestrator:Spawn", "Project Root Directory: {}", projectRootDir.string());
        LNF_INFO("Lunalify:Orchestrator:Spawn", "Final command: {}", winrt::to_string(cmdLine));
        LNF_DEBUG("Lunalify:Orchestrator:Spawn", "Trying to spawn in directory: {}", binDir.string());
        LNF_DEBUG("Lunalify:Orchestrator:Spawn", "Final command line: {}", winrt::to_string(cmdLine));

        // Process creation
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        BOOL success = CreateProcessW(
            NULL,
            const_cast<LPWSTR>(cmdLine.c_str()),
            NULL,
            NULL,
            FALSE,
            CREATE_NO_WINDOW| CREATE_BREAKAWAY_FROM_JOB,
            NULL,
            projectRootDir.c_str(), // use project root as working directory
            &si,
            &pi
        );

        if (success) {
            AssignProcessToJobObject(g_hOrchestratorJob, pi.hProcess);
            LNF_INFO("Lunalify:Orchestrator:Spawn", "Daemon process spawned. PID: {}", pi.dwProcessId);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return Errors::Code::SUCCESS;
        }

        LNF_ERROR("Lunalify:Orchestrator:Spawn", "Failed to launch process. Error: {}", GetLastError());
        return Errors::Code::IPC_DAEMON_NOT_FOUND;
    }

    Errors::Code Orchestrator::Run(DWORD ppid) {
        if (m_running) return Errors::Code::SUCCESS;

        LNF_INFO("Lunalify:Orchestrator:Run", "Initializing Runtime Environment...");

        try {
            // Initalizes the WinRT apartment for the daemon process
            winrt::init_apartment(winrt::apartment_type::multi_threaded);
        } catch (...) {
            LNF_ERROR("Lunalify:Orchestrator:Run", "Failed to initialize WinRT Apartment.");
            return Errors::Code::WINRT_INIT_FAILED;
        }

        m_running = true;
        // Initialize the event thread (outbound: daemon -> lua)
        m_eventThread = std::thread(&Orchestrator::EventLoop, this);
        // Initialize the command thread (inbound: lua -> daemon)
        m_commandThread = std::thread(&Orchestrator::CommandLoop, this);
        SignalReadyToParent(ppid);

        LNF_INFO("Lunalify:Orchestrator:Run", "Threads started. Entering Message Loop...");

        MSG msg;
        while (m_running && GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        return Errors::Code::SUCCESS;
    }

    void Orchestrator::Stop() {
        m_running = false;
        // Post a blank message to unblock GetMessage if it's waiting
        PostQuitMessage(0);

        if (m_commandThread.joinable()) m_commandThread.join();
        if (m_eventThread.joinable()) m_eventThread.join();

        LNF_INFO("Lunalify:Orchestrator:Stop", "Runtime stopped.");
    }

    void Orchestrator::CommandLoop() {
        PipeServer cmdPipe(Protocol::CMD_PIPE_NAME, PipeServer::Mode::Duplex);
        while (m_running) {
            if (cmdPipe.Listen() != Errors::Code::SUCCESS) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }
            while (m_running) {

            LNF_DEBUG("Lunalify:Orchestrator:CommandLoop", "Client connected to Command Pipe!");

            Protocol::PacketHeader header;
            // Tente ler o header
            auto res = cmdPipe.ReadExact(reinterpret_cast<uint8_t*>(&header), sizeof(header));

            if (res == Errors::Code::SUCCESS) {
                // LNF_DEBUG("Lunalify:Orchestrator:CommandLoop", "Packet received! Magic: 0x{:X}, Op: {}, Size: {}",
                //          header.magic, (uint32_t)header.opCode, header.payloadSize);

                if (header.magic == Protocol::MAGIC_HEADER) {
                    std::string payload;
                    if (header.payloadSize > 0) {
                        payload.resize(header.payloadSize);
                        cmdPipe.ReadExact(reinterpret_cast<uint8_t*>(payload.data()), header.payloadSize);
                    }

                    // Send the ACK immediately after reading the payload, before processing the Toast
                    // This releases the Lua/Client process to continue its life.
                    uint8_t ack = static_cast<uint8_t>(Protocol::OpCode::Acknowledge);
                    cmdPipe.Write({ ack });

                    // Now process the command (if the dispatch takes long, the Lua process is not blocked)
                    this->HandleCommand(header.opCode, payload);
                } else {
                    LNF_ERROR("Lunalify:Orchestrator:CommandLoop", "Invalid Magic Header: 0x{:X}", header.magic);
                }
            } else {
                LNF_WARN("Lunalify:Orchestrator:CommandLoop", "Failed to read header from client.");
            }
            }

            cmdPipe.Disconnect();
        }
    }


    void Orchestrator::EventLoop() {
        PipeServer eventPipe(Protocol::EVENT_PIPE_NAME, PipeServer::Mode::Outbound);
        // bool signaled = false;

        while (m_running) {
            if (eventPipe.Listen() != Errors::Code::SUCCESS) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            // Signal the parent process that the event pipe is ready
            // if (!signaled) {
            //     SignalReadyToParent(ppid);
            //     signaled = true;
            // }

            while (m_running) {
                // ToastEvent ev;
                auto evOpt = m_eventQueue.TryPop();
                if (!evOpt.has_value()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    continue;
                }
                auto ev = evOpt.value();
                std::string payload = ev.eventName + Protocol::SEPARATOR + ev.toastId + Protocol::SEPARATOR + ev.args;
                auto packet = Protocol::Pack(Protocol::OpCode::EVENT_DISPATCH, payload);
                if (eventPipe.Write(packet) != Errors::Code::SUCCESS) {
                    break;
                }
                LNF_DEBUG("Lunalify:Orchestrator:EventLoop", "Event dispatched: {}", ev.eventName);
            }
            eventPipe.Disconnect();
        }
    }

    void Orchestrator::HandleCommand(Protocol::OpCode op, const std::string& payload) {
        if (payload.empty()) return;
        std::stringstream ss(payload);
        switch (op) {
            case Protocol::OpCode::CMD_FIRE_TOAST: {
                std::string appId, toastId, pTitle, pValue, pDisplay, pStatus, xml;
                LNF_DEBUG("Lunalify:Orchestrator", "Payload size: {} bytes", payload.size());
                if (!std::getline(ss, appId, Protocol::SEPARATOR) ||
                    !std::getline(ss, toastId, Protocol::SEPARATOR) ||
                    !std::getline(ss, pTitle, Protocol::SEPARATOR) ||
                    !std::getline(ss, pValue, Protocol::SEPARATOR) ||
                    !std::getline(ss, pDisplay, Protocol::SEPARATOR) ||
                    !std::getline(ss, pStatus, Protocol::SEPARATOR))
                    {
                        LNF_ERROR("Lunalify:Orchestrator:HandleCommand", "Malformed payload: Could not parse all fields.");
                        return;
                    }
                size_t currentPos = static_cast<size_t>(ss.tellg());
                if (currentPos < payload.size()) {
                    xml = payload.substr(currentPos);
                }
                if (xml.empty()) {
                    LNF_ERROR("Lunalify:Orchestrator:HandleCommand", "XML payload is empty!");
                    return;
                }
                LNF_INFO("Lunalify:Orchestrator:HandleCommand", "Firing delegated toast: {}", toastId);
                // Callback that will connect the UI interaction with event queue in orchestrator
                auto onInteraction = [this](std::string id, std::string event, std::string args) {
                    this->m_eventQueue.Push(id, event, args);
                };
                std::wstring wXml = Lunalify::Utils::to_wstring(xml);
                Errors::Code res = Notifications::Notifier::Dispatch(
                    appId, wXml, toastId, pTitle, pValue, pDisplay, pStatus, onInteraction
                );
                if (res != Errors::Code::SUCCESS) {
                    LNF_ERROR("Lunalify:Orchestrator:HandleCommand", "Failed to dispatch toast: {}", (int)res);
                    m_eventQueue.Push(toastId, "failed", std::to_string((int)res));
                }
            }
            break;
            case Protocol::OpCode::CMD_SHUTDOWN: {
                LNF_INFO("Lunalify:Orchestrator:HandleCommand", "Remote shutdown requested.");
                this->m_running = false;
                break;
            }

            case Protocol::OpCode::CMD_UPDATE_TOAST: {
                LNF_DEBUG("Orchestrator", "Raw Payload received: [{}]", payload);
                std::string appId, tag, title, value, vString, status;
                std::stringstream ss(payload);
                std::getline(ss, appId, Protocol::SEPARATOR);
                std::getline(ss, tag, Protocol::SEPARATOR);
                std::getline(ss, title, Protocol::SEPARATOR);
                std::getline(ss, value, Protocol::SEPARATOR);
                std::getline(ss, vString, Protocol::SEPARATOR);
                std::getline(ss, status, Protocol::SEPARATOR);
                if (appId.empty() || tag.empty()) {
                    LNF_ERROR("Lunalify:Orchestrator:HandleCommand", "Malformed payload: AppId or Tag is missing.");
                    break;
                }
                LNF_DEBUG("Lunalify:Orchestrator:HandleCommand", "Updating toast: {}", tag);
                Notifications::Notifier::Update(appId, tag, title, value, vString, status);
                break;
            }

            default:
                LNF_WARN("Lunalify:Orchestrator:HandleCommand", "Unknown OpCode received: {}", (int)op);
                break;
        }
    }

    // void Orchestrator::SignalReadyToParent(DWORD ppid) {
    //     if (ppid == 0) return;
    //     std::wstring eventName = Lunalify::Globals::GlobalLunalifyReadyEvent(ppid);
    //     HANDLE hEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, eventName.c_str());
    //     if (hEvent) {
    //         SetEvent(hEvent);
    //         CloseHandle(hEvent);
    //         LNF_INFO("Lunalify:Orchestrator:SignalReadyToParent", "Parent process {} signaled.", ppid);
    //     }
    // }
    void Orchestrator::SignalReadyToParent(DWORD ppid) {
        if (ppid == 0) return;

        std::wstring eventName = Lunalify::Globals::GlobalLunalifyReadyEvent(ppid);
        HANDLE hEvent = NULL;

        // Try to open the event with retry (up to 2 seconds)
        for (int i = 0; i < 20; i++) {
            hEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, eventName.c_str());
            if (hEvent) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (hEvent) {
            SetEvent(hEvent);
            CloseHandle(hEvent);
            LNF_INFO("Lunalify:Orchestrator:SignalReady", "Signal sent to PPID {}", ppid);
        } else {
            // if reach here, the event name generated in Daemon is different from Lua or an error occurred
            LNF_ERROR("Lunalify:Orchestrator:SignalReady", "FAILED to open event: {}", winrt::to_string(eventName));
        }
    }
}
