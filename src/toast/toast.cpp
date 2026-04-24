#include "../../include/lunalify/lunalify.hpp"
#include "handleapi.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Notifications.h>

using namespace winrt::Windows::UI::Notifications;

Lunalify::Errors::Code send_toast(const ToastConfig& config, const std::string& uniqueId) {
    winrt::init_apartment(winrt::apartment_type::multi_threaded);
    try {
        auto hAppId = winrt::to_hstring(config.appId);
        // auto hAppId = winrt::to_hstring(L"Lunalify.TEST");
        // auto notifier = ToastNotificationManager::CreateToastNotifier(L"Lunalify.Dev.Test");
        auto notifier = ToastNotificationManager::CreateToastNotifier(hAppId);
        auto setting = notifier.Setting();
        if (setting != NotificationSetting::Enabled) {
            LNF_WARN("App:Toast", "Toast blocked by Windows settings for AppID: {}. Status: {}", config.appId, (int)setting);
            switch (setting) {
                case NotificationSetting::DisabledForUser:
                    return Lunalify::Errors::Code::WINRT_DISBLED_BY_USER;
                case NotificationSetting::DisabledByGroupPolicy:
                    return Lunalify::Errors::Code::WINRT_DISABLED_BY_POLICY;
                case NotificationSetting::DisabledForApplication:
                    return Lunalify::Errors::Code::WINRT_REGISTRATION_MISSING;
                default:
                    // Unknown setting, return generic error
                    return Lunalify::Errors::Code::UNKNOWN;
            }
        }
    } catch (winrt::hresult_error const& ex) {
        std::string errMsg = winrt::to_string(ex.message());
        LNF_ERROR("App:Toast", "WinRT Exception: {} (0x{:X})", errMsg, (uint32_t)ex.code().value);
        if (ex.code() == winrt::hresult(0x80070490)) { // Element Not Found
            LNF_WARN("App:Toast", "It's possible that Windows hasn't indexed AppId '{}' yet. Bypassing sync check.", config.appId);
            // return Lunalify::Errors::Code::SUCCESS;
        } else {
            LNF_ERROR("App:Toast", "WinRT Exception: {} (0x{:X})", errMsg, (uint32_t)ex.code().value);
            return Lunalify::Errors::Code::WINRT_INIT_FAILED;
        }
    } catch (...) {
        LNF_ERROR("App:Toast", "Unknown Critical Exception in WinRT check.");
        return Lunalify::Errors::Code::WINRT_INIT_FAILED;
    }

    std::wstring xml = Lunalify::BuildToastXml(config);
    std::string xmlStr = winrt::to_string(xml);


    LNF_INFO("App:Toast", "Attempting to connect to Daemon command pipe.");
    HANDLE hPipe = CreateFileW(Lunalify::Protocol::CMD_PIPE_NAME,
            GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hPipe == INVALID_HANDLE_VALUE) {
            DWORD err = GetLastError();
            LNF_ERROR("App:Toast", "Failed to connect to pipe. Error: {}", err);

            if (err == ERROR_FILE_NOT_FOUND) return Lunalify::Errors::Code::IPC_DAEMON_NOT_FOUND;
            if (err == ERROR_PIPE_BUSY) return Lunalify::Errors::Code::IPC_PIPE_BUSY;
            if (err == ERROR_ACCESS_DENIED) return Lunalify::Errors::Code::SYS_ACCESS_DENIED;

            return Lunalify::Errors::Code::IPC_CONNECTION_LOST;
        }

        LNF_INFO("App:Toast", "Daemon command pipe connection successful.");
        std::string payload = Lunalify::Protocol::CMD_FIRE_TOAST + Lunalify::Protocol::SEPARATOR +
                              config.appId + Lunalify::Protocol::SEPARATOR +
                              uniqueId + Lunalify::Protocol::SEPARATOR +
                              config.progress.title + Lunalify::Protocol::SEPARATOR +
                              config.progress.value + Lunalify::Protocol::SEPARATOR +
                              config.progress.displayValue + Lunalify::Protocol::SEPARATOR +
                              config.progress.status + Lunalify::Protocol::SEPARATOR +
                              xmlStr;

        DWORD written;
        BOOL success = WriteFile(hPipe, payload.c_str(), (DWORD)payload.size() + 1, &written, NULL);
        if (success) {
            // Ensure the bytes are delivered to the Daemon before continuing
            FlushFileBuffers(hPipe);
            // Wait for Daemon ack (keeps the JOB alive)
            char ack;
            DWORD read;
            /*BOOL result =*/ ReadFile(hPipe, &ack, 1, &read, NULL);
            // if (result) {
            //     LNF_DEBUG("App:Toast", "Received ACK from Daemon for toast {}.", uniqueId);
            // }

        }
        CloseHandle(hPipe);
        if (!success || written == 0) {
            LNF_ERROR("App:Toast", "Failed to write to pipe.");
            return Lunalify::Errors::Code::IPC_CONNECTION_LOST;
        }
        LNF_INFO("App:Toast", "Delegated toast {} to Daemon.", uniqueId);
        return Lunalify::Errors::Code::SUCCESS;
}
