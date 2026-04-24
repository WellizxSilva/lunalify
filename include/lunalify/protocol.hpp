#pragma once
#include <string>

namespace Lunalify::Protocol {
    inline const wchar_t* CMD_PIPE_NAME = L"\\\\.\\pipe\\LunalifyCommands";
    inline const wchar_t* EVENT_PIPE_NAME = L"\\\\.\\pipe\\LunalifyEvents";

    // Command strings sent from Main Lua -> Daemon
    inline const std::string CMD_FIRE_TOAST = "FIRE";
    inline const std::string CMD_CLOSE_TOAST = "CLOSE";
    inline const std::string CMD_EXIT = "EXIT";
    inline const std::string CMD_UPDATE_TOAST = "UPDATE";

    // Event strings sent from Daemon -> Main Lua
    inline const std::string EVENT_ACTIVATED = "ACTV";
    inline const std::string EVENT_DISMISSED = "DSMS";

    const char SEPARATOR = '|';

    struct RawMessage {
        std::string command;
        std::string appId;
        std::string toastId;
        std::string data;

        std::string serialize() const {
            return command + SEPARATOR + appId + SEPARATOR + toastId + SEPARATOR + data;
        }
    };
}
