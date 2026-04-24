local Errors = require('lunalify.errors')
local CONFIGS = {}

local CONSTANTS = {}
CONFIGS.CONSTANTS = CONSTANTS

CONSTANTS.SOUNDS = {
    DEFAULT  = "Default",
    IM       = "IM",
    MAIL     = "Mail",
    REMINDER = "Reminder",
    SMS      = "SMS",
    -- Alarms (Looping)
    ALARM    = "Looping.Alarm",
    ALARM2   = "Looping.Alarm2",
    ALARM3   = "Looping.Alarm3",
    ALARM4   = "Looping.Alarm4",
    ALARM5   = "Looping.Alarm5",
    ALARM6   = "Looping.Alarm6",
    ALARM7   = "Looping.Alarm7",
    ALARM8   = "Looping.Alarm8",
    ALARM9   = "Looping.Alarm9",
    ALARM10  = "Looping.Alarm10",
    -- Calls (Looping)
    CALL     = "Looping.Call",
    CALL2    = "Looping.Call2",
    CALL3    = "Looping.Call3",
    CALL4    = "Looping.Call4",
    CALL5    = "Looping.Call5",
    CALL6    = "Looping.Call6",
    CALL7    = "Looping.Call7",
    CALL8    = "Looping.Call8",
    CALL9    = "Looping.Call9",
    CALL10   = "Looping.Call10"
}

CONSTANTS.DURATION = {
    SHORT = "short",
    LONG = "long"
}

CONSTANTS.SCENARIO = {
    DEFAULT = "default",
    ALARM = "alarm",
    REMINDER = "reminder",
    INCOMING_CALL = "incomingCall"
}


CONSTANTS.Errors = Errors.ENUMS

return CONFIGS
