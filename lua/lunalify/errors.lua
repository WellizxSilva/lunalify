local constants_location = 'Lunalify.CONSTANTS'
local Errors = {
    CODES = {
        SUCCESS = 0,
        FILE_NOT_FOUND = 1,
        INVALID_APP_ID = 2,
        POWERSHELL_NOT_FOUND = 3,
        TEMPLATE_NOT_SUPPORTED = 4,
        TOAST_NOT_SUPPORTED = 5,
        INVALID_SOUND = 6,
        INVALID_DURATION = 7,
        INVALID_SCENARIO = 8,
    },

    MESSAGES = {
        [1] = "File not found or permission denied.",
        [2] = "Invalid or unregistered AppID.",
        [3] = "PowerShell executable not found in system PATH.",
        [4] = "Template 'ToastGeneric' is not supported on this version of Windows.",
        [5] = "Too many actions. Windows limits toasts to 5 buttons (3 recommended).",
        [6] = "Invalid sound. Check " .. constants_location .. ".SOUNDS",
        [7] = "Invalid duration. Check " .. constants_location .. ".DURATION",
        [8] = "Invalid scenario. Check " .. constants_location .. ".SCENARIOS",
        [9] = "Missing display_name parameter when unregistering an app.",
        ["UNKNOWN"] = "Unknown error while triggering notification."
    }
}

--- Throws an error with the given code and details, using the specified level
---
--- @param code number The error code to throw
--- @param details any Optional details to include in the error message
--- @param level number? The error level (1 = print, 2 = error)
function Errors.throw(code, details, level)
    local msg = Errors.MESSAGES[code] or Errors.MESSAGES["UNKNOWN"]
    local output = string.format("[Lunalify Error %d] %s", code, msg)
    if details then
        output = output .. " | Details: " .. tostring(details)
    end
    local levelMap = {
        [1] = print,
        [2] = error,
    }
    local func = levelMap[level] or print
    func(output)
end

local LunalifyCoreErrors = {}
Errors.LUNALIFY_CORE_Errors = LunalifyCoreErrors

LunalifyCoreErrors.type = {
    SYSTEM   = "SYSTEM",
    IPC      = "IPC",
    WINRT    = "WINRT",
    INTERNAL = "INTERNAL"
}

LunalifyCoreErrors.CODES = {
    -- System
    [101] = { name = "SYS_PATH_NOT_FOUND", type = LunalifyCoreErrors.type.SYSTEM },
    [102] = { name = "SYS_ACCESS_DENIED", type = LunalifyCoreErrors.type.SYSTEM },
    [103] = { name = "SYS_SHORTCUT_FAILED", type = LunalifyCoreErrors.type.SYSTEM },
    [104] = { name = "SYS_INVALID_ENCODING", type = LunalifyCoreErrors.type.SYSTEM },
    -- IPC
    [201] = { name = "IPC_PIPE_BUSY", type = LunalifyCoreErrors.type.IPC },
    [202] = { name = "IPC_DAEMON_NOT_FOUND", type = LunalifyCoreErrors.type.IPC },
    [203] = { name = "IPC_CONNECTION_LOST", type = LunalifyCoreErrors.type.IPC },
    [204] = { name = "IPC_TIMEOUT", type = LunalifyCoreErrors.type.IPC },
    [205] = { name = "IPC_PROTOCOL_MISMATCH", type = LunalifyCoreErrors.type.IPC },
    -- WinRT
    [301] = { name = "WINRT_INIT_FAILED", type = LunalifyCoreErrors.type.WINRT },
    [302] = { name = "WINRT_INVALID_XML", type = LunalifyCoreErrors.type.WINRT },
    [303] = { name = "WINRT_REGISTRATION_MISSING", type = LunalifyCoreErrors.type.WINRT },
    [304] = { name = "WINRT_DISBLED_BY_USER", type = LunalifyCoreErrors.type.WINRT },
    [305] = { name = "WINRT_DISABLED_BY_POLICY", type = LunalifyCoreErrors.type.WINRT },
    -- [306] = { name = "WINRT_QUIET_HOURS", type = LunalifyCoreErrors.type.WINRT },
    [307] = { name = "WINRT_QUEUE_FULL", type = LunalifyCoreErrors.type.WINRT },
    -- Internal
    [901] = { name = "INTERNAL_DAEMON_CRASH", type = LunalifyCoreErrors.type.INTERNAL },
    [902] = { name = "INTERNAL_MEMORY_ERR", type = LunalifyCoreErrors.type.INTERNAL },
    [999] = { name = "UNKNOWN", type = LunalifyCoreErrors.type.INTERNAL }
}

function LunalifyCoreErrors.format(code, message)
    local info = LunalifyCoreErrors.CODES[code] or LunalifyCoreErrors.CODES[999]
    return {
        success = false,
        code = code,
        name = info.name,
        type = info.type,
        message = message or "No details provided."
    }
end

Errors.ENUMS = {
    -- SYSTEM
    SYS_PATH_NOT_FOUND = 101,
    SYS_ACCESS_DENIED = 102,
    SYS_SHORTCUT_FAILED = 103,
    SYS_INVALID_ENCODING = 104,

    -- IPC
    IPC_PIPE_BUSY = 201,
    IPC_DAEMON_NOT_FOUND = 202,
    IPC_CONNECTION_LOST = 203,
    IPC_TIMEOUT = 204,
    IPC_PROTOCOL_MISMATCH = 205,

    -- WINRT
    WINRT_INIT_FAILED = 301,
    WINRT_INVALID_XML = 302,
    WINRT_REGISTRATION_MISSING = 303,
    WINRT_DISABLED_BY_USER = 304,
    WINRT_DISABLED_BY_POLICY = 305,
    -- WINRT_QUIET_HOURS = 306,
    WINRT_QUEUE_FULL = 307,

    -- INTERNAL
    INTERNAL_DAEMON_CRASH = 901,
    INTERNAL_MEMORY_ERR = 902,
    UNKNOWN = 999,
}

return Errors
