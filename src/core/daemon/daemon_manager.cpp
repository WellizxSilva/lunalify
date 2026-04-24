#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Notifications.h>
#include <string>
#include <thread>
#include "../../../include/lunalify/ToastEventManager.hpp"
#include "winbase.h"
#include <namedpipeapi.h>
#include <stdio.h>
#include <fstream>
#include <chrono>
#include <iomanip>
#include "../../../include/lunalify/lunalify.hpp"
extern HINSTANCE g_hInstDLL;

// Job handler that runs while the dll is loaded
static HANDLE g_hDaemonJob = NULL;

namespace Lunalify::Daemon {
        void HandleIncomingCommand(const std::string& raw) {
            std::stringstream ss(raw);
            std::string cmd;
            if (!std::getline(ss, cmd, Lunalify::Protocol::SEPARATOR)) {
               return;
            }
            // std::getline(ss, cmd, Lunalify::Protocol::SEPARATOR);
            // LNF_DEBUG("App:Daemon", "Token [CMD]: {}", cmd);

            if (cmd == Lunalify::Protocol::CMD_FIRE_TOAST) {
                std::string appId, toastId, pTitle, pValue, pDisplay, pStatus, xml;
                    std::getline(ss, appId, Lunalify::Protocol::SEPARATOR);
                    std::getline(ss, toastId, Lunalify::Protocol::SEPARATOR);
                    std::getline(ss, pTitle, Lunalify::Protocol::SEPARATOR);
                    std::getline(ss, pValue, Lunalify::Protocol::SEPARATOR);
                    std::getline(ss, pDisplay, Lunalify::Protocol::SEPARATOR);
                    std::getline(ss, pStatus, Lunalify::Protocol::SEPARATOR);
                    // Take the rest of the buffer as XML content
                    xml = ss.str().substr((size_t)ss.tellg());

                LNF_INFO("App:Daemon:Command", "Firing delegated toast for Id: {}", toastId);
                // Call WinRT in the daemon process
                Lunalify::Errors::Code result = Lunalify::DispatchXmlToast(appId, to_wstring(xml.c_str()), toastId, pTitle, pValue, pDisplay, pStatus);
                if (result != Lunalify::Errors::Code::SUCCESS) {
                    std::string errorMsg = Lunalify::Errors::GetMessage(result);
                    LNF_ERROR("App:Daemon:Command", "Toast failed: {}", errorMsg);
                    ToastEventManager::PushEvent(toastId, "failed", errorMsg);
                }
            }
            else if( cmd == Lunalify::Protocol::CMD_UPDATE_TOAST) {
                std::string appId, tag, title, value, vString, status;
                LNF_DEBUG("Daemon:Raw", "Raw buffer received: [{}]", raw);
                    std::getline(ss, appId, Lunalify::Protocol::SEPARATOR);
                    std::getline(ss, tag, Lunalify::Protocol::SEPARATOR);
                    std::getline(ss, title, Lunalify::Protocol::SEPARATOR);
                    std::getline(ss, value, Lunalify::Protocol::SEPARATOR);
                    std::getline(ss, vString, Lunalify::Protocol::SEPARATOR);
                    std::getline(ss, status, Lunalify::Protocol::SEPARATOR);
                LNF_DEBUG("App:Daemon", "Updating Toast Tag: {} to {}%", tag, value);
                Lunalify::UpdateToastData(appId, tag, title, value, vString, status);
            }

            else if (cmd == Lunalify::Protocol::CMD_EXIT) {
               LNF_INFO("App:Daemon:Command", "Shutdown command received. Exiting...");
                exit(0);
            }
        }


        void SignalReadyToParent(DWORD parentPid) {
            if (parentPid == 0) return;

            // Build the event name using the parent process PID
            std::wstring eventName = Lunalify::Globals::globalLunalifyReadyEvent(parentPid);

            // Try to open the event created by the parent process
            HANDLE hEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, eventName.c_str());
            if (hEvent) {
                LNF_INFO("App:Daemon", "Signaling parent process {} that we are ready.", parentPid);
                SetEvent(hEvent); // Releases the WaitForSingleObject from the parent process
                CloseHandle(hEvent);
            } else {
                LNF_WARN("App:Daemon", "Could not open ready event. Error: {}", GetLastError());
            }
        }


    void StartEventPipeServer(DWORD ppid) {
        bool signaled = false;
        while (true) {
            LNF_INFO("App:Daemon:StartEventPipeServer", "Creating Named Pipe...");
            HANDLE hPipe = CreateNamedPipeW(Lunalify::Protocol::EVENT_PIPE_NAME,
                        PIPE_ACCESS_OUTBOUND, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                        1, 4096, 4096, 0, NULL);

            if (hPipe == INVALID_HANDLE_VALUE) {
                LNF_WARN("App:Daemon:StartEventPipeServer", "Failed to create pipe. Error: {}", GetLastError());
                Sleep(100);
                continue;
            }

            if (!signaled && ppid != 0) {
                SignalReadyToParent(ppid);
                signaled = true;
            }

            LNF_INFO("App:Daemon:StartEventPipeServer", "Pipe created. Waiting for Lua connection...");
            if (ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
                // char buffer[4096];
                LNF_INFO("App:Daemon:StartEventPipeServer", "Lua connected. Checking events...");
                while (true) {
                    ToastEvent ev;

                    // Blocks until the WinRT process pushes an event to the queue
                    if (ToastEventManager::PopEvent(ev)) {
                        LNF_INFO("App:Daemon:StartEventPipeServer", "Event received: {}", ev.eventName);
                        std::string payload = ev.eventName + Lunalify::Protocol::SEPARATOR +
                                                             ev.toastId + Lunalify::Protocol::SEPARATOR +
                                                             ev.args;
                        DWORD written;
                        BOOL success = WriteFile(hPipe, payload.c_str(), (DWORD)payload.size(), &written, NULL);
                        FlushFileBuffers(hPipe);

                        if (!success || written == 0) {
                            LNF_WARN("App:Daemon", "Client disconnected. Breaking pipe.");
                            break;
                        }
                        LNF_INFO("Daemon:Events", "Dispatched event: {} for ID: {}", ev.eventName, ev.toastId);
                    }
                }
            }
            DisconnectNamedPipe(hPipe); // Release for next connection
            CloseHandle(hPipe);
        }
    }

    void StartCommandServer() {
            while (true) {
                LNF_INFO("App:Daemon:StartCommandServer", "Creating command pipe...");
                HANDLE hPipe = CreateNamedPipeW(Lunalify::Protocol::CMD_PIPE_NAME,
                    PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_WAIT | PIPE_READMODE_MESSAGE,
                    1, 8192, 8192, 0, NULL);
                LNF_INFO("App:Daemon:StartCommandServer", "Command pipe created.");

                if (ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
                    LNF_INFO("App:Daemon:StartCommandServer", "Client connected.");
                    char buffer[8192];
                    DWORD bytesRead;
                    ZeroMemory(buffer, sizeof(buffer));
                    if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
                        buffer[bytesRead] = '\0';
                        HandleIncomingCommand(std::string(buffer));
                        char ack = 1;
                        DWORD bytesWritten;
                        if (WriteFile(hPipe, &ack, 1, &bytesWritten, NULL)) {
                            FlushFileBuffers(hPipe); // Ensures the byte is sent before continuing
                            LNF_DEBUG("App:Daemon:StartCommandServer", "ACK sent to client.");
                        } else {
                            LNF_WARN("App:Daemon:StartCommandServer", "Failed to send ACK. Error: {}", GetLastError());
                        }
                    }
                    LNF_INFO("App:Daemon:StartCommandServer", "Client disconnected.");
                }
                DisconnectNamedPipe(hPipe);
                CloseHandle(hPipe);
            }
        }


    void Run(DWORD ppid) {
        LNF_INFO("App:Daemon:Run", "Initializing WinRT Multi-threaded...");
        try {
            //WinRT needs to be initialized with multi-threaded apartment type
            winrt::init_apartment(winrt::apartment_type::multi_threaded);
            LNF_INFO("App:Daemon:Run", "WinRT initialized successfully.");
            std::thread commandThread(StartCommandServer);
            commandThread.detach(); // Use a detached thread for the command server
            LNF_INFO("App:Daemon:Run", "Command server started.");

            std::thread eventThread([ppid]() {
                StartEventPipeServer(ppid);
            });
            eventThread.detach();
            LNF_INFO("App:Daemon:Run", "Starting Pipe Server...");

            // SignalReadyToParent(ppid);
            MSG msg;
            while (GetMessage(&msg, NULL, 0, 0)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        } catch (winrt::hresult_error& ex) {
            LNF_ERROR("App:Daemon:Run", "Critical WinRT error: {}", winrt::to_string(ex.message()));
            return;
        }
    }

    Lunalify::Errors::Code Spawn(const std::string& logPath, DWORD parentPid) {
        if (g_hDaemonJob == NULL) {
            g_hDaemonJob = CreateJobObjectW(NULL, NULL);
            if (g_hDaemonJob) {
                JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = { };
                // Kill all processes in this job when the job is closed
                jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
                SetInformationJobObject(g_hDaemonJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));
            }
        }
        // Mutex to ensure unique instance
        HANDLE hMutex = CreateMutexW(NULL, TRUE, L"Local\\LunalifyDaemonMutex");
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            if (hMutex) CloseHandle(hMutex);
            return Lunalify::Errors::Code::SUCCESS;
        }
        if (WaitNamedPipeW(Lunalify::Protocol::EVENT_PIPE_NAME, 100)) {
            LNF_INFO("App:Daemon:Spawn", "Pipe Server already online.");
            return Lunalify::Errors::Code::SUCCESS;
        }


        wchar_t dllPath[MAX_PATH];
        if (GetModuleFileNameW(g_hInstDLL, dllPath, MAX_PATH) == 0) {
            return Lunalify::Errors::Code::IPC_DAEMON_NOT_FOUND;
        }
        std::filesystem::path p(dllPath);
        std::filesystem::path binDir = p.parent_path();
        std::filesystem::path lunalifyDir = binDir.parent_path();
        std::filesystem::path projectRootDir = lunalifyDir.parent_path();
        if (projectRootDir.empty()) projectRootDir = std::filesystem::current_path();
        std::wstring logArg = L"";
        std::wstring wLogPathForDebug = L"DISABLED";
        if (!logPath.empty()) {
            std::wstring wLogPath = to_wstring(logPath.c_str());
            logArg = L" --daemon-log \"" + wLogPath + L"\"";
            wLogPathForDebug = wLogPath;
        }


        // std::wstring dllPathStr(dllPath);
        // size_t lastSlash = dllPathStr.find_last_of(L"\\/");
        // std::wstring binDir = dllPathStr.substr(0, lastSlash);

        // size_t secondLastSlash = binDir.find_last_of(L"\\/");
        // std::wstring lunalifyFolder = binDir.substr(0, secondLastSlash);
        // // up two levels to get the project root dir
        // size_t thirdLastSlash = lunalifyFolder.find_last_of(L"\\/");
        // std::wstring projectRootDir = lunalifyFolder.substr(0, thirdLastSlash);

        // if (projectRootDir.empty()) projectRootDir = L".";
        // LNF_INFO("App:Daemon:Spawn", "Project Root Directory: {}", winrt::to_string(projectRootDir));


        // std::filesystem::path p(dllPath);
        // std::filesystem::path lunalifyDir = p.parent_path().parent_path();
        // std::filesystem::path projectRootDir = lunalifyDir.parent_path();
        // std::filesystem::path defaultLogPath = lunalifyDir / "logs" / "daemon.log";
        // std::wstring logArg = L"";
        // if (!logPath.empty()) {
        //     std::wstring wLogPath = to_wstring(logPath.c_str());
        //     logArg = L" --daemon-log \"" + wLogPath + L"\"";
        // }
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);


        std::wstring cmd = L"\"" + std::wstring(exePath) +
            L"\" -e \"package.path='.;./?.lua;./?/init.lua;'..package.path\"" +
            L" -l lunalify -- --lunalify-daemon" +
            logArg +
            L" --ppid " + std::to_wstring(parentPid);
        LNF_INFO("App:Daemon:Spawn", "Project Root Directory: {}", projectRootDir.string());
        LNF_INFO("App:Daemon:Spawn", "Final command: {}", winrt::to_string(cmd));
        // LNF_INFO("App:Daemon:Spawn", "Trying to spawn in directory: {}", winrt::to_string(binDir));
        LNF_DEBUG("App:Daemon:Spawn", "Log path: {}", winrt::to_string(wLogPathForDebug));

        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        if (CreateProcessW(
            NULL,
            const_cast<LPWSTR>(cmd.c_str()),
            NULL,
            NULL,
            FALSE,
            CREATE_NO_WINDOW, // No console
            NULL,
            projectRootDir.c_str(),
            &si,
            &pi
        ))
        {
            if (g_hDaemonJob) {
                AssignProcessToJobObject(g_hDaemonJob, pi.hProcess);
            }
            LNF_INFO("App:Daemon:Spawn", "Daemon spawned and assigned to Job Object.");
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return Lunalify::Errors::Code::SUCCESS;
        } else {
            LNF_ERROR("App:Daemon:Spawn", "CreateProcessW failed. Error: {}", GetLastError());
            return Lunalify::Errors::Code::IPC_DAEMON_NOT_FOUND;
        }
    }
}
