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
    
    -- Stop the script if the toast was dismissed
    if ev.event == "dismissed" then
        return false 
    end
end)
```

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
