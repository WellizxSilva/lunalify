#pragma once
#include <string>

namespace Lunalify {
    namespace Errors {
        enum class Code : int {
            SUCCESS = 0,
            // --- [100-199] System and Environment Errors ---
            SYS_PATH_NOT_FOUND       = 101,
            SYS_ACCESS_DENIED        = 102,
            SYS_SHORTCUT_FAILED      = 103,
            SYS_INVALID_ENCODING     = 104,

            // --- [200-299] IPC (Inter-Process Communication) Errors ---
            IPC_PIPE_BUSY           = 201,
            IPC_DAEMON_NOT_FOUND     = 202,
            IPC_CONNECTION_LOST      = 203,
            IPC_TIMEOUT              = 204,
            IPC_PROTOCOL_MISMATCH    = 205,

            // --- [300-399] WinRT and Notification Errors ---
            WINRT_INIT_FAILED        = 301,
            WINRT_INVALID_XML        = 302,
            WINRT_REGISTRATION_MISSING = 303,
            WINRT_DISBLED_BY_USER    = 304,
            WINRT_DISABLED_BY_POLICY  = 305,
            // WINRT_QUIET_HOURS        = 306,
            WINRT_QUEUE_FULL         = 307,

            // --- [900-999] Critical Internal Errors ---
            INTERNAL_DAEMON_CRASH    = 901,
            INTERNAL_MEMORY_ERR      = 902,
            UNKNOWN                      = 999
        };

        inline std::string TranslateCode(Code code) {
            switch (code) {
                case Code::SUCCESS:             return "Success";
                // System
                case Code::SYS_PATH_NOT_FOUND:   return "Path not found or directory creation failed";
                case Code::SYS_ACCESS_DENIED:    return "Permission denied to write file or shortcut";
                case Code::SYS_SHORTCUT_FAILED:  return "Failed to create shortcut";
                case Code::SYS_INVALID_ENCODING: return "Invalid encoding detected";

                // IPC
                case Code::IPC_PIPE_BUSY:        return "Pipe is busy or locked by another process";
                case Code::IPC_DAEMON_NOT_FOUND: return "Lunalify Daemon is not running or pipe is closed";
                case Code::IPC_CONNECTION_LOST:  return "Connection to Lunalify Daemon was lost";
                case Code::IPC_TIMEOUT:          return "Operation timed out";
                case Code::IPC_PROTOCOL_MISMATCH: return "Protocol mismatch with Lunalify Daemon";

                // WinRT
                case Code::WINRT_INIT_FAILED:        return "RoInitialize/MTA failed";
                case Code::WINRT_INVALID_XML:        return "The provided Toast XML is invalid or malformed";
                case Code::WINRT_REGISTRATION_MISSING: return "AppID not found in Windows registry";
                case Code::WINRT_DISBLED_BY_USER:    return "Notifications for this app are disabled in Windows settings";
                case Code::WINRT_DISABLED_BY_POLICY:  return "Blocked by group policy (GPO/Company)";
                // case Code::WINRT_QUIET_HOURS:        return "Windows is in Focus Mode (Quiet Hours)";
                case Code::WINRT_QUEUE_FULL:         return "Excessive notifications in Windows queue";

                // Critical
                case Code::INTERNAL_DAEMON_CRASH:    return "Lunalify Daemon crashed unexpectedly";
                case Code::INTERNAL_MEMORY_ERR:      return "Memory allocation failed";
                case Code::UNKNOWN:                  return "An unknown error occurred";
                default: return "Unknown Error";
            }
        }
    }
}
