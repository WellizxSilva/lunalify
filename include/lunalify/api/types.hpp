#pragma once
#include "lunalify/logger.hpp"
#include <string>
#include <vector>

namespace Lunalify::API::Toast {

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
    std::string tag;
    std::string group;
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
    bool silent = false;
};

struct ToastUpdate {
    std::string appId;
    std::string tag;
    std::string title;
    std::string value;
    std::string vString;
    std::string status;
};

}

namespace Lunalify::App::Lua::DataStructs {
    struct RegisterData {
        std::string appId;
        std::string appName;
        std::string iconPath;
        std::string daemonLogPath;
    };
    struct LoggerData {
        bool enabled;
        Lunalify::LogLevel level;
        std::string path;
        bool to_console;
    };
}
