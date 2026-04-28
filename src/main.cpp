#define _WIN32_WINNT 0x0A00
#define NTDDI_VERSION NTDDI_WIN10_CO
#include <sdkddkver.h>
#if !defined(_WIN32)
    #error "This library is only supported on Windows."
#endif

#if NTDDI_VERSION < NTDDI_WIN10_CO
    #error "This library is only supported on Windows 11 Build 22000 or later."
#endif

#include "lunalify/api/api_context.hpp"
// #include <shellapi.h>
using namespace Lunalify::API;


// Enables the DLL to intercept the process if it is the Daemon
extern "C" BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        // Disable thread library calls to avoid deadlocks (std::thread is used internally)
        DisableThreadLibraryCalls(hModule);
        auto& ctx = Context::Get();
        ctx.SetDllInstance(hModule);

        // if (wcsstr(GetCommandLineW(), L"--lunalify-daemon")) {
        //     ctx.SetIsDaemon(true);
        //      HandleDaemonMode(); // Entry point for daemon mode
        // }
    }
    return TRUE;
}
