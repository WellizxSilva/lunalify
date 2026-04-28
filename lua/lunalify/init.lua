--- @class Lunalify.API
--- @field data table
--- @field _generated_id string|nil
--- @field _pending_events table|nil
--- @field _dispatch function
--- @field _callbacks table
local API = {}
API.__index = API
--- @alias Lunalify.Error { code: number, name: string, type: 'SYSTEM' | 'IPC' | 'WINRT' | 'INTERNAL', message: string }
--- @alias Lunalify.Kit.Setup { app_id: string | nil, app_name: string, icon_path: string, hero_path: string, sound: string, silent: boolean, strict: boolean, duration: 'short' | 'long', scenario: string, logging: { enabled: boolean, level: 1 | 2 | 3, app_log_path: string, daemon_log_path: string, to_console: boolean } }
--- @alias Lunalify.API.Action { type: "protocol" | "foreground"| "background", placement?: "default" | "contextMenu", style?: 'Success' | 'Critical', image?: string, id?: string}
--- @alias Lunalify.API.FIRE.executor fun(title: string, message: string, opts: table, dispatch: fun(title: string, message: string, opts: table))
--- @alias Lunalify.API.Selection.SelectionTable { id: string, content: string }
---@class Lunalify.ActivatedArgs
---@field action string The Id of the button clicked or "default" if the toast body was clicked.
---@field [string] string Any additional input values from text fields or selections, indexed by their Id.

--- @alias Lunalify.API.EventCallback fun(args: Lunalify.ActivatedArgs|string)
--- @alias Lunalify.API.Event.DismissedReason 'UserCanceled' | 'Timeout' | 'ApplicationHidden'
--- @alias Lunalify.API.Event.EventName 'activated' | 'dismissed' | 'failed'
--- @alias Lunalify.API.Listen.Callback.Event { id: string, event: Lunalify.API.Event.EventName, args: Lunalify.ActivatedArgs, reason?: Lunalify.API.Event.DismissedReason }
--- @alias Lunalify.API.Listen.Callback fun(ev: Lunalify.API.Listen.Callback.Event): any


local lunalify = {}
--- Exposes the Lunalify Kit module.
lunalify.kit = {}
lunalify._registered_ids = lunalify._registered_ids or {}
local registered_ids = lunalify._registered_ids
--- Hook System
local EventRegistry = {}

lunalify._VERSION = "0.1.0"
local info = debug.getinfo(1, "S")
local script_path = info.source:sub(2):gsub("\\", "/")

local parent_dir = script_path:match("^(.*)/lunalify/init%.lua$")
if not parent_dir or parent_dir == "" then
    parent_dir = "."
end
local pattern1 = parent_dir .. "/?.lua;"
local pattern2 = parent_dir .. "/?/init.lua;"
if not package.path:find(pattern1, 1, true) then
    package.path = pattern1 .. pattern2 .. package.path
end
local base_dir = script_path:match("(.*[/\\])") or ""
local dll_path = base_dir .. "bin/lunalify_core.dll"

local core
local success, res = pcall(require, "lunalify_core")

if success then
    core = res
else
    -- If requires fall here, it means that the library was not installed via LuaRocks, and should find in the local bin folder
    local loader, err = package.loadlib(dll_path, "luaopen_lunalify_core")
    if loader then
        core = loader()
    else
        error("Lunalify: Native core not found.\n" ..
            "Attempted path: " .. dll_path .. "\n" ..
            "Require: " .. tostring(res) .. "\n" ..
            "Loadlib: " .. tostring(err))
    end
end

local Helpers = require("lunalify.helpers")
local Errors = require("lunalify.errors")
local CONFIGS = require("lunalify.configs")
lunalify.CONSTANTS = CONFIGS.CONSTANTS
local LNF_SOUNDS = lunalify.CONSTANTS.SOUNDS
local LNF_SCENARIO = lunalify.CONSTANTS.SCENARIO
local LNF_DURATION = lunalify.CONSTANTS.DURATION



local settings = {
    app_id = "Microsoft.Windows.Explorer",
    app_name = "Lunalify App",
    icon_path = "",
    hero_path = "",
    sound = "Default",
    silent = false,
    strict = true,
    duration = "short",
    scenario = "default",
    logging = {
        enabled = true,
        level = 1,
        app_log_path = "",
        daemon_log_path = "",
        to_console = true
    }
}

--- Internal function to dispatch a notification.
local function _dispatch(title, message, options)
    options = options or {}
    local current_app_id = options.app_id or settings.app_id
    local current_app_name = options.app_name or settings.app_name
    local current_icon = options.icon_path or settings.icon_path
    local current_hero = options.hero_path or settings.hero_path
    local daemon_log_path = options.daemon_log_path or settings.logging.daemon_log_path
    local final_daemon_log_path
    if daemon_log_path then
        final_daemon_log_path = Helpers.Path.resolve_path(daemon_log_path, false)
    end
    if not registered_ids[current_app_id] then
        core.register(current_app_id, current_app_name, current_icon or "", final_daemon_log_path)
        registered_ids[current_app_id] = true
    end

    local icon, icon_ok = Helpers.Path.resolve_path(current_icon)
    local hero, hero_ok = Helpers.Path.resolve_path(current_hero or "")

    if settings.strict then
        if current_icon and not icon_ok then return nil, "Invalid icon path: " .. current_icon, 101 end
        if current_hero and not hero_ok then return nil, "Invalid hero path: " .. current_hero, 101 end
    end
    -- returns the toast id
    return
        core.toast({
            title = title,
            message = message,
            appId = options.app_id or settings.app_id,
            iconPath = icon,
            heroPath = hero,
            sound = options.sound or "Default",
            silent = options.silent or false,
            duration = options.duration,
            scenario = options.scenario,
            actions = options.actions or {},
            inputs = options.inputs or {},
            progress = options.progress or {},
            tag = options.tag or "",
            group = options.group or ""
        })
end


--- Registers an app Id with the system for toast notifications.
--- @param app_id string The App Id to register.
--- @param display_name string The display name of the app.
--- @param icon_path string? (optional) Path to the notification icon (will be resolved).
--- @param daemon_log_path string? (optional) Path to the daemon log file.
--- @return boolean success
--- @return Lunalify.Error|nil error
function lunalify.kit.register(app_id, display_name, icon_path, daemon_log_path)
    local final_icon_path
    local final_daemon_log_path
    if icon_path then
        final_icon_path = Helpers.Path.resolve_path(icon_path)
    end
    if daemon_log_path then
        final_daemon_log_path = Helpers.Path.resolve_path(daemon_log_path, false)
    end
    local ok, err_msg, err_code = core.register({
        id = app_id,
        name = display_name,
        iconPath = final_icon_path,
        daemonLogPath = final_daemon_log_path
    })
    registered_ids[app_id] = true
    if not ok then
        return false, Errors.format(err_code, err_msg)
    end
    return true
end

--- Unregisters an app Id from the system and removes its shortcut.
--- @param app_id string The App Id to unregister.
--- @param display_name string The display name of the app (required to easily locate the .lnk file).
--- @return boolean success
--- @return Lunalify.Error|nil error
function lunalify.kit.unregister(app_id, display_name)
    if not display_name or display_name == "" then
        return false, Errors.throw(9)
    end
    local ok, err_msg, err_code = core.unregister(app_id, display_name)
    if not ok then
        return false, Errors.LUNALIFY_CORE_Errors.format(err_code, err_msg)
    end
    return true
end

-- function lunalify.proxy(payload)
--     if type(payload) ~= "table" then return false end
--     return core.send_notification(payload)
-- end


local function __bridge_event(ev)
    if ev and ev.id and EventRegistry[ev.id] then
        local registry = EventRegistry[ev.id]
        local callback = registry[ev.event]
        if callback then
            local final_args = ev.args
            if ev.event == "activated" then
                final_args = Helpers._parse_payload(ev.args)
            elseif ev.event == "failed" then
                local error_code = tonumber(ev.args) or 999 -- (999 = unknown)
                final_args = Errors.LUNALIFY_CORE_Errors.format(error_code,
                    "Async notification failure reported by Daemon.")
            end
            callback(final_args)
        end

        -- Automatically clear the registry entry when the toast is dismissed or activated (avoid memory leaks)
        if ev.event == "dismissed" or ev.event == "failed" then
            EventRegistry[ev.id] = nil
        end
    end
end

--[[
####################################
############### API #################
#####################################
--]]


lunalify.API = API

--- Create a new notification
--- @param title string The title of the notification
--- @param message string The message of the notification
--- @param opts table The options for the notification
--- @param dispatch_fn function The dispatch function for the notification
--- @return Lunalify.API
function API.new(title, message, opts, dispatch_fn)
    local self = setmetatable({}, API)
    local merged = {}
    for k, v in pairs(opts) do
        merged[k] = v
    end

    self.data = {
        title   = title,
        message = message,
        opts    = merged,
        actions = merged.actions or {},
        -- inputs           = merged.inputs or {},
        -- progress         = merged.progress or {},
    }
    self._callbacks = {}
    self._dispatch = dispatch_fn -- Ref to dispatch function
    self._generated_id = nil
    return self
end

--- Add an interactive action (button) to the notification.
--- @param label string Text displayed on the button.
--- @param target string URL or command executed when clicked.
--- @param opts Lunalify.API.Action? Optional action settings
--- @return Lunalify.API
function API:action(label, target, opts)
    opts = opts or {}
    if #self.data.actions >= 5 then
        --- @diagnostic disable-next-line: return-type-mismatch
        return Errors.throw(5, nil, 2)
    end
    local final_image = nil
    if opts.image then
        local path, ok = Helpers.Path.resolve_path(opts.image)
        if not ok then
            -- if failed, Errors.throw was already called in resolve_path
            return self
        end
        final_image = path
    end

    table.insert(self.data.actions,
        {
            label = label,
            target = target,
            type = opts.type or "background",
            placement = opts.placement or "default",
            image = final_image,
            id = opts.id or label,
            style = opts.style,
        })
    return self
end

--- Set the icon for the notification.
--- @param path string Path to the icon file. (MUST BE A VALID PATH)
--- @return Lunalify.API
function API:icon(path)
    self.data.opts.icon_path = path
    return self
end

--- Configure the notification sound.
--- @param sound_type string|boolean Sound type (see lunalify.CONSTANTS.SOUNDS).
---   - Pass false to mute the notification.
--- @return Lunalify.API
function API:sound(sound_type)
    if sound_type == false then
        self.data.opts.silent = true
        self.data.opts.sound = LNF_SOUNDS.DEFAULT -- Reset to default, but don't play
    else
        self.data.opts.silent = false
        self.data.opts.sound = sound_type or LNF_SOUNDS.DEFAULT
    end
    return self
end

--- Configure the duration of the notification.
--- @param mode 'long' | 'short' ||boolean The duration mode (see lunalify.CONSTANTS.DURATION for available options)
--- - use true for long duration
--- @return Lunalify.API
function API:duration(mode)
    if mode == LNF_DURATION.LONG or mode == true then
        self.data.opts.duration = LNF_DURATION.LONG
    else
        self.data.opts.duration = LNF_DURATION.SHORT
    end
    return self
end

--- Set the scenario type for the notification.
--- @param type 'alarm' | 'reminder' | 'incomingCall' | 'default' The scenario type (see lunalify.CONSTANTS.SCENARIO for available options)
--- @return Lunalify.API
function API:scenario(type)
    self.data.opts.scenario = type or LNF_SCENARIO.DEFAULT
    return self
end

--- Add a text input field to the notification.
--- @param id string Unique identifier for the input.
--- @param placeholder string|nil Hint text displayed inside the input.
--- @param title string|nil Small title text above the input.
--- @return Lunalify.API
function API:input(id, placeholder, title)
    self.data.opts.inputs = self.data.opts.inputs or {}
    table.insert(self.data.opts.inputs, {
        id = id,
        type = "text",
        placeholder = placeholder or "",
        title = title or "",
        defaultInput = "",
        selections = {}
    })
    return self
end

-- Add a selection (dropdown) menu to the notification.
--- @param id string Unique identifier for the selection.
--- @param selections_table Lunalify.API.Selection.SelectionTable Array of options
--- @param default_id string|nil The ID of the option selected by default.
--- @param title string|nil Small title text above the dropdown.
--- @return Lunalify.API
function API:selection(id, selections_table, default_id, title)
    self.data.opts.inputs = self.data.opts.inputs or {}
    table.insert(self.data.opts.inputs, {
        id = id,
        type = "selection",
        placeholder = "",
        title = title or "",
        defaultInput = tostring(default_id or ""),
        selections = selections_table or {}
    })
    return self
end

--- Configure a progress bar for the notification.
--- @param status string|nil Status text (e.g., "Downloading...").
--- @param value number|string|nil Progress value (0.0 to 1.0) or "indeterminate".
--- @param display_value string|nil Custom string to show over the bar (e.g., "45%"). If nil, the value is shown as a default Windows percentage.
--- @param title string|nil Title for the progress section.
--- @return Lunalify.API
function API:progress(status, value, display_value, title)
    local final_value = tostring(value or "0")
    if value ~= "indeterminate" then
        local n = tonumber(value)
        if not n or n < 0 then
            final_value = "0"
        elseif n > 1 then
            final_value = "1.0"
        end
    end
    self.data.opts.progress = {
        active = true,
        title = title or "",
        value = final_value,
        displayValue = tostring(display_value or ""),
        status = status or ""
    }
    return self
end

-- o id da toast vira a tag
--- Set the tag for the toast notification. (***The tag will be used as the toast's id, so avoid using the same tag for different notifications.***)
--- if you want to replace an existing toast with the same tag, use the same tag string.
--- @param tag_string string|nil The tag string to set.
--- @return Lunalify.API
function API:tag(tag_string)
    self.data.opts.tag = tostring(tag_string or "")
    return self
end

--- Set the group for the toast notification.
--- @param group_string string|nil The group string to set.
--- @return Lunalify.API
function API:group(group_string)
    self.data.opts.group = tostring(group_string or "")
    return self
end

--- Dispatch the notification to the system.
--- This method finalizes the configuration and triggers the dispatch function.
--- @param executor Lunalify.API.FIRE.executor? Optional executor function to handle the notification.
--- Note that this method must be called after all configuration methods.
---  - The toast execution is an asynchronous process. (that is, it does not block the Lua execution and can take some time to execute
--- @return string|nil id Unique notification Id on success, or nil on failure.
--- @return Lunalify.Error|nil error Error object if dispatch fails
function API:fire(executor)
    self.data.opts.actions = self.data.actions
    -- self.data.opts.actions = self.data.opts.actions


    local function handle_dispatch()
        local preferred_id = self.data.opts.tag -- if provided, use it as the notification Id
        local id, err_msg, err_code = self._dispatch(self.data.title, self.data.message, self.data.opts)
        if err_msg or err_code then
            local error_obj = Errors.LUNALIFY_CORE_Errors.format(err_code, err_msg)
            return nil, error_obj
        end
        self._generated_id = preferred_id or id
        if not self._generated_id then
            return nil, Errors.LUNALIFY_CORE_Errors.format(301)
        end

        -- Tranfer events pending to the global registry indexed by the real Id
        EventRegistry[self._generated_id] = self._pending_events or {}
        self._pending_events = nil
        return id, nil
    end

    if type(executor) == "function" then
        return executor(self.data.title, self.data.message, self.data.opts, handle_dispatch)
    end

    return handle_dispatch()
end

--- Register a callback for a specific event.
--- @overload fun(self: Lunalify.API, eventName: Lunalify.API.Event.EventName, callback: Lunalify.API.EventCallback)
-- @overload fun(self: Lunalify.API, eventName: '"dismissed"', callback: fun(reason: Lunalify.API.Event.DismissedReason))
-- @overload fun(self: Lunalify.API, eventName: '"failed"', callback: fun(error: string))
--- @param eventName Lunalify.API.Event.EventName The name of the event to listen for.
--- @param callback Lunalify.API.EventCallback The callback function to execute when the event is triggered.
--- @return Lunalify.API
function API:on(eventName, callback)
    if self._generated_id then
        EventRegistry[self._generated_id] = EventRegistry[self._generated_id] or {}
        EventRegistry[self._generated_id][eventName] = callback
    else
        self._pending_events = self._pending_events or {}
        self._pending_events[eventName] = callback
    end
    return self
end

--- Updates an existing notification's progress data.
--- @param value number|string Progress value (0.0 to 1.0) or "indeterminate".
--- @param status string|nil New status text.
--- @param display_value string|nil New custom display string.
--- @param title string|nil New title for the progress section.
--- @return boolean success
function API:update(value, status, display_value, title)
    if not self._generated_id then
        -- Cannot update a notification that has not been dispatched yet.
        return false
    end
    local p = self.data.opts.progress
    if p then
        if value then p.value = tostring(value) end
        if status then p.status = status end
        if display_value then p.displayValue = display_value end
        if title then p.title = title end
    end
    return core.update_toast({
        appId = self.data.opts.app_id or settings.app_id,
        tag = self.data.opts.tag or self._generated_id,
        title = p.title or "",
        value = p.value or "0",
        displayValue = p.displayValue or "",
        status = p.status or "",
    })
end

--- Register a callback for the "failed" event.
--- @param callback fun(err: Lunalify.Error) The callback function to execute when the event is triggered.
function API:onError(callback)
    self._pending_events = self._pending_events or {}
    self._pending_events["failed"] = callback
    return self
end

-- ########## KIT ##########


--- Run a loop (listening for events) automatically to process callbacks (This is a blocking function)
--- @param callback Lunalify.API.Listen.Callback? Function that will be called before each iteration to check if the loop should stop. (should return `false` to stop)
--- @return any
function lunalify.kit.listen(callback)
    local function log(...)
        if settings.logging.enabled then
            print("[Lunalify:Listen] ", ...)
        end
    end
    log('Event loop started')
    while true do
        local ev = core.wait_event()

        -- if callback and callback() == false then break end
        -- Block the execution until an event is received

        if ev then
            __bridge_event(ev)
            if callback and callback(ev) == false then break end
            log("Event received from Daemon: " .. ev.event)
        else
            log("Connection closed or timeout.")
            break -- Closed pipe or Daemon error
        end
    end
    log("Event loop ended.")
end

--- Shutdown the Lunalify kit and send a shutdown signal to the Daemon.
--- - It's highly recommended to call this function before exiting your application.
function lunalify.kit.shutdown()
    local function log(...)
        if settings.logging.enabled then
            print("[Lunalify:Shutdown] ", ...)
        end
    end
    log("Sending shutdown signal to Daemon...")
    local ok = core.shutdown()
    if ok then
        log("Shutdown signal sent successfully.")
    else
        log("Failed to send shutdown signal. Daemon not found or already shut down.")
    end
end

-- ##########################


--- Lunalify Namespace
lunalify.toast = {
    --- Create a new toast notification instance using the Windows Runtime Api (WinRT)
    --- @param title string The title of the notification.
    --- @param msg string The body text of the notification.
    create = function(title, msg)
        return API.new(title, msg, settings, _dispatch)
    end
}



local _initialized = false
--- Configure global settings for Lunalify.
--- This function applies user-provided configuration values to the internal settings table.
---
--- @param config Lunalify.Kit.Setup A table of configuration values.
--- @return boolean success
--- @return Lunalify.Error|nil error
function lunalify.kit.setup(config)
    if _initialized then return false end -- Avoid re-initialization
    for k, v in pairs(config) do
        if k == "icon_path" or k == "hero_path" then
            -- simple validation to satisfy the interpreter (v is a string)
            assert(type(v) == "string", "icon_path and hero_path must be strings") --
            v = Helpers.Path.resolve_path(v)
        end
        settings[k] = v
    end

    if config.logging then
        for k, v in pairs(config.logging) do
            settings.logging[k] = v
        end
    end
    local log = settings.logging
    local final_app_log_path = ""
    if log.app_log_path then
        final_app_log_path = Helpers.Path.resolve_path(log.app_log_path, false)
    end
    core.set_logger(log.enabled, log.level, final_app_log_path, log.to_console)
    -- -- Ensure the default app id is registered
    local ok, err = lunalify.kit.register(settings.app_id, settings.app_name, settings.icon_path, log.daemon_log_path)
    if not ok then
        return false, err
    end
    _initialized = true
    return true
end

-- Setup global configuration
-- lunalify.setup({})


--[[
####################################
####################################
#####################################
--]]


return lunalify
