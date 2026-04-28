#include "lunalify/lunalify.hpp"
#include "lunalify/api/api_context.hpp"
#include "lunalify/runtime/orchestrator.hpp"
#include "lunalify/runtime/handlers/handle_daemon.hpp"
#include <shellapi.h>

namespace Lunalify::Runtime::Handlers {
    void HandleDaemonMode() {
        LPWSTR* szArglist;
        int nArgs;
        szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
        std::string logPath = "";
        DWORD ppid = 0;
        bool logEnabled = false;
        if (szArglist) {
            for (int i = 0; i < nArgs; i++) {
                if (std::wstring(szArglist[i]) == L"--daemon-log" && (i + 1) < nArgs) {
                    std::wstring val = szArglist[++i];
                    logPath = std::string(val.begin(), val.end());
                    logEnabled = true;
                }
                if (std::wstring(szArglist[i]) == L"--ppid" && (i + 1) < nArgs) {
                    ppid = std::wcstoul(szArglist[++i], nullptr, 10);
                }
            }
            LocalFree(szArglist);
        }
        Lunalify::Logger::Initialize(logEnabled, Lunalify::LogLevel::LEVEL_DEBUG, logPath, false);
        if (ppid == 0) {
            LNF_ERROR("Lunalify:Daemon:Handler", "Critical: PPID is 0. Cannot signal parent!");
            ExitProcess(1);
        }
        Lunalify::Runtime::Orchestrator orchestrator;
        auto result = orchestrator.Run(ppid);
        if (result != Lunalify::Errors::Code::SUCCESS) {
            LNF_ERROR("Lunalify:Daemon:Handler", "Orchestrator failed to start: {}", Lunalify::Errors::TranslateCode(result));
            orchestrator.Stop(); // Ensure clean shutdown even if orchestrator fails to start
            ExitProcess((UINT)result);
        }
        LNF_INFO("Lunalify:Daemon:Handler", "Shutting down...");
        orchestrator.Stop();
        ExitProcess(0);
    }
}
