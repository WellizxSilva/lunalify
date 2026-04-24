#pragma once
#ifndef LUNALIFY_HPP
#define LUNALIFY_HPP

#include <string>
#include <windows.h>
#include <vector>
#include <queue>
#include <mutex>
#include "ToastEventManager.hpp"
#include "protocol.hpp"
#include "logger.hpp"
#include "errors.hpp"

extern "C" {
    #include <lua.h>
    #include <lauxlib.h>
}

struct ToastSelection {
    std::string id;
    std::string content;
};

struct ToastProgress {
    bool active = false;
    std::string title;
    std::string value;
    std::string displayValue;
    std::string status;
};

struct ToastInput {
    std::string id;
    std::string type;
    std::string placeholder;
    std::string title;
    std::string defaultInput;
    std::vector<ToastSelection> selections;
};

struct ToastAction {
    std::string label;
    std::string target;
    std::string type;
    std::string placement;
    std::string image;
    std::string style;
};

struct ToastConfig {
    //
    std::string tag;
    std::string group;
    //
    std::string title;
    std::string message;
    std::string appId;
    std::string iconPath;
    std::string heroPath;
    std::string sound;
    std::string duration;
    std::string scenario;
    std::vector<ToastAction> actions;
    std::vector<ToastInput> inputs;
    ToastProgress progress;
    bool silent;
};


namespace Lunalify {

    namespace Globals {
        inline std::wstring globalLunalifyReadyEvent(DWORD pid) {
            return L"Global\\LunalifyReady_" + std::to_wstring(pid);
        }
    }

    // Helper for escaping strings
    std::string Escape(const std::string& input);

    std::wstring BuildToastXml(const ToastConfig& config);

    Lunalify::Errors::Code UpdateToastData(const std::string& appId, const std::string& tag, const std::string& title, const std::string& value, const std::string& vString, const std::string& status);
    Lunalify::Errors::Code DispatchXmlToast(const std::string& appId, const std::wstring& xmlContent, const std::string& uniqueId, const std::string& initialTitle, const std::string& initialVal, const std::string& initialDisplay, const std::string& initialStatus);
}

namespace Lunalify::Daemon {
    void StartEventPipeServer();
    void Run(DWORD ppid);
    Lunalify::Errors::Code Spawn(const std::string& logPath, DWORD parentPid);
}

// Internal functions
Lunalify::Errors::Code register_app_identity(const std::wstring& wAppId, const std::wstring& wAppName);
Lunalify::Errors::Code unregister_app_identity(const std::wstring& wAppId, const std::wstring& shortcutPath);
Lunalify::Errors::Code create_app_shortcut(const std::wstring& appId,const std::wstring& exePath, const std::wstring& shortcutPath, const std::wstring& iconPath);
Lunalify::Errors::Code send_toast(const ToastConfig& config, const std::string& uniqueId);
std::wstring to_wstring(const char* str);
void WriteLog(const std::string& text);

#endif
