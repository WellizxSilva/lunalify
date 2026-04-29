# Kit Reference

The `kit` namespace manages the lifecycle of the Lunalify Daemon, communication pipes, and global configurations. It is the core engine of the library

## Configuration & Registration

### `lunalify.kit.setup(config)`
Initializes the library with a configuration table. It resolves paths, sets up logging, and automatically registers the application with the Windows OS.

**Arguments:**
- `config` (table): A table containing the following optional fields:

| Field | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `app_id` | string | `"Lunalify.Framework"` | Unique identifier for Windows (AUMID). |
| `app_name` | string | `"Lunalify App"` | Name displayed in the Action Center. |
| `icon_path` | string | `""` | Relative or absolute path to the global app icon. |
| `hero_path` | string | `""` | Relative or absolute path to a global hero (banner) image. |
| `sound` | string | `"Default"` | Default sound for notifications. |
| `silent` | boolean | `false` | If true, notifications won't make a sound by default. |
| `duration` | string | `"short"` | Global notification duration (`"short"` or `"long"`). |
| `scenario` | string | `"default"` | Global scenario type (`"default"`, `"alarm"`, etc). |
| `strict` | boolean | `true` | If true, throws errors for invalid file paths before sending. |
| `logging` | table | *(see below)* | Advanced configuration for the C++ Daemon logging. |

**The `logging` Table Configuration:**
| Field | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `enabled` | boolean | `true` | Enables/disables internal C++ logging. |
| `level` | number | `1` | Log verbosity (1 = Info, 2 = Warning, 3 = Error). |
| `to_console` | boolean | `true` | Mirrors log output to the standard console. |
| `app_log_path` | string | `""` | Path to save the main Lua bridge logs. |
| `daemon_log_path` | string | `""` | Path to save the isolated background Daemon logs. |

**Example:**

```lua
lunalify.kit.setup({
    app_id = "MyApp.Id",
    app_name = "My App",
    icon_path = "./assets/my_app_logo.png",
    logging = {
        enabled = true,
        to_console = false,
        app_log_path = "./logs/myapp.lunalify.log",
        daemon_log_áth = "./logs/myapp.lunalify.daemon.log"
    }
})
```

### `lunalify.kit.register(app_id, display_name, icon_path)`
Manually registers an Application id. This is called internally by setup.

- `app_id` (string): The AUMID to register.
- `display_name` (string): Name seen by the user.
- `icon_path` (string, optional): Path to the icon. (.ico, .lnk)
- `daemon_log_path` (string, optional): Path to store background daemon logs.

---

### `lunalify.kit.unregister(app_id, display_name)`
Removes the application's identity registration from the system and deletes the associated shortcut.

- `app_id` (string): The AUMID that was previously registered.
- `display_name` (string): The display name used during registration. Required to locate the physical file in the Start Menu.


> [!IMPORTANT]
Why is `display_name` **required for unregistration?** > On Windows, native toast notification validation depends on the existence of a shortcut file `(.lnk)` in the user's Programs folder. The filename of this shortcut is generated based on the `display_name.` Without it, the framework would be able to clean up the Registry keys but would leave an "orphan shortcut" in the user's Start Menu, leading to UI inconsistencies and potential errors during future registrations.

**Usage Example**
```lua
local app_id = "MyProject.Lunalify.App"
local display_name = "My Native App"

-- To completely remove the app from the notification system:
local success, err = lunalify.kit.unregister(app_id, display_name)
if not success and err then
    print("Failed to unregister: " .. err.message)
end
```
---

## Lifecycle Management

### `lunalify.kit.shutdown()`
Sends a termination signal (`EXIT`) to the Daemon and closes all open named pipes. 
> **Note:** `Lunalify` uses *Windows Job Objects* to automatically kill the Daemon if the parent Lua process crashes. However, calling shutdown() manually before exiting your script is the strictly recommended practice for a clean exit.

### `lunalify.kit.sleep(ms)`
Suspends the execution of the current Lua thread for a specified number of milliseconds. This is a `native binding` to the Windows `Sleep` API, meaning it consumes `0% CPU while waiting`, unlike busy-wait loops.
  - `ms` (number): The time to sleep in milliseconds (1 second = 1000ms).

> [!TIP]
`Note:` This is a helper function designed primarily for standalone or CLI scripts. If you are using an external GUI framework (like `Qt`, `LÖVE`, or `Neovim`), you should use the framework's native timer/sleep functions instead to avoid blocking the event loop of the UI.
```lua
while app_running do
    lunalify.kit.update()
    
    -- Using the native sleep to keep CPU usage near zero
    lunalify.kit.sleep(10) 
end
```

---

## Event Handling
The `kit` manages the core event loop. This loop listens to the Daemon and automatically dispatches events to the specific Toast callbacks (registered via `toast:on()`).

### 1. `lunalify.kit.listen(callback)`
**Mode:** Blocking (Synchronous).
This is the recommended way to handle interactions. It pauses the script execution and waits for Windows to send a signal (click or dismissal).
- **callback** (function, optional): A global interceptor function that receives every `event` table. Returning `false` inside the callback breaks the infinite loop.
```lua
lunalify.kit.listen(function(ev)
    print("Global Event Intercepted: " .. ev.event)
    
    -- Stop the loop if the toast was dismissed
    if ev.event == "dismissed" then
        return false 
    end
end)
```

### 2. `lunalify.kit.update(callback)`
**Mode:** Non-Blocking (Asynchronous).
Processes all pending events in the communication buffer using **Overlapped I/O**. This function must be called repeatedly within your own Main Loop or via a high-frequency timer.

- **Advantage:** Allows your script to continue executing other tasks (animations, game logic, background processing) while waiting for user interactions.
- **Usage:** Should be called frequently (e.g., every 10ms to 100ms) to ensure responsiveness.

- **callback:** (function, optional): A function called for each event retrieved in the current batch. If the callback returns `false`, `update()` will stop processing further events in the buffer and return immediately, even if more events are pending.
```lua
-- Example usage in a custom loop
while my_app_running do
    lunalify.kit.update(function(ev)
        -- Stop the loop if the toast was dismissed
        if ev.event == 'dismissed' then
            return false
        end
    end)
    -- Always use a sleep to prevent 100% CPU usage in while loops
    lunalify.kit.sleep(16)
end
```

### 3. `lunalify.kit.watch(event_name, callback)`
- **Mode:** Asynchronous / Global Monitor.
Registers a global observer for a specific event type. Unlike `toast:on()`, watchers registered here are persistent and will trigger for every notification processed by the framework.
- **event_name** (string): `"activated"`, `"dismissed"`, or `"failed"`.
- **callback** (function): Receives the full `ev` object.
```lua
-- Useful for global logic
lunalify.kit.watch("failed", function(ev)
    print("ALERT: Notification " .. ev.id .. " has failed!")
end)
```

---

## The Event Object
Whether intercepted globally via `kit.listen(ev)` or handled locally via `toast:on("event", function(args))`, the core event data contains:

| File | Type   | Description                                |
|------|--------|--------------------------------------------|
| name | string | The action performed ("activated", "dismissed", or "failed").       |
| id   | string | The unique system Id of the Toast notification.                    |
| args | table/string | The launcher arguments or action arguments | The parsed payload containing user interactions.

### Understanding ev.args (The Payload)
When the event is `"activated"` (the user clicked a button or the toast), Lunalify automatically parses the XML payload into a Lua Table.
```lua
-- Example of ev.args when a button with inputs is clicked:
{
    action = "exec_cleanup",      -- The Id of the clicked button
    admin_note = "Do it fast",    -- Value typed in the text input (mapped by input Id)
    cleanup_method = "zip"        -- Value selected in the dropdown (mapped by selection Id)
}
```
> If the event is `"dismissed"` or `"failed"`, args will typically be a simple string indicating the reason (e.g.,` "UserCanceled"`, `"TimedOut"`, or the error message).
