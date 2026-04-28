#pragma once

#include <string>
#include <windows.h>
#include <vector>
#include <queue>
#include <mutex>

#include "errors.hpp"
#include "logger.hpp"
#include "protocol.hpp"
#include "utils/utils.hpp"

// extern "C" {
//     #include <lua.h>
//     #include <lauxlib.h>
// }

namespace Lunalify {
    namespace Globals {
        inline std::wstring GlobalLunalifyReadyEvent(DWORD pid) {
            return L"Global\\LunalifyReady_" + std::to_wstring(pid);
        }
    }
    namespace Locals {
        inline std::wstring LocalLunalifyOrchestratorMutex() {
            return L"Local\\LunalifyOrchestratorMutex";
        }
    }
}
