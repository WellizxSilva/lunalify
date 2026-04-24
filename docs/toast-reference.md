# Toast Reference

The `toast` namespace is used to construct and fire notifications. Lunalify uses a **Fluent Interface (Builder Pattern)**, allowing you to chain methods to customize your Toast with ease.

---

## Creating a Toast
### `lunalify.toast.create(title, message)`
Starts the construction of a new Toast instance. 
- **title** (string): The bold header text.
- **message** (string): The main body text.
- **Returns**: A `ToastBuilder` instance.

---

## Configuration Methods (Builder Pattern)
These methods return the builder object, allowing for method chaining:

| Method | Argument(s) | Description |
| :--- | :--- | :--- |
| `:icon(path)` | string | Sets the small icon. Lunalify resolves the path automatically. |
| `:sound(name)` | string | Overrides the global sound setting. Pass `false` to mute. |
| `:duration(type)`| string/boolean| Sets how long it stays: `"short"` (default) or `"long"` (`true`).|
| `:scenario(type)` | string | Sets priority: "default", "alarm", "reminder", "incomingCall"|
| `:progress(status, value, display, title)` | string, number, string, string | Adds a progress bar to the Toast. |
| `:input(id, placeholder, title)` | string, string, string | Adds a text input field. |
| `:selection(id, selections, default, title)` | string, table, string, string | Adds a dropdown menu. |
| `:action(label, target, opts)` | string, string, table | Adds a button to the Toast (Max 5). |
| `:on(event, callback)` | string, function | Attaches an event listener (`"activated"`, `"dismissed"`, etc). |
| `:onError(callback)` | function | Attaches an error callback specifically for dispatch failures. |
| `:fire(executor)` | function (optional) | Dispatches the XML payload to the Daemon. |
---

## Interactive Actions
The `:action()` method allows you to add up to 5 buttons (3 recommended) to your notification. It accepts a label, a target, and an optional table for advanced configuration.

### `:action(label, target, opts)`
- label (string): The text displayed on the button.
- target (string): The argument or URL returned to the event loop (or opened by the system).
- opts (table, optional): Advanced settings for the action:

| Option | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `type` | string | `"background"` | Use `"protocol"` for URLs or `"background"/"foreground"` for app events. |
| `placement` | string | `"default"` | `"default"` (button) or `"contextMenu"` (hidden in right-click menu). |
| `image` | string | `nil` | Path to an icon displayed next to the button text. |
| `style` | string | `nil` | **Windows 11 Native Colors:** `"Success"` (Green) or `"Critical"` (Red). |
| `id` | string | `label` | A unique identifier for the action. |

Action Examples
1. Simple Button
```lua
toast:action("Check Details", "btn_details")
```

2. Colored Button with Icon (Windows 11)
Using `Success` or `Critical` styles provides an excellent native UI feel.
```lua
toast:action("Clean Now", "exec_cleanup", { 
    style = "Success", 
    image = "./icons/check.png" 
})
toast:action("Ignore", "exec_ignore", { 
    style = "Critical", 
    image = "./icons/cancel.png" 
})
```

2. Button with Icon and Custom Id
Adding an icon to a button makes the UI feel more integrated with Windows 11.
```lua
toast:action("Check Details", "cmd_details", {
    image = "./assets/info-icon.png",
    id = "btn_details"
})
```

3. Context Menu Action
Actions with `placement = "contextMenu"` appear when the user right-clicks the notification.
```lua
toast:action("Settings", "open_settings", { placement = "contextMenu" })
```

## Rich UI Elements (Inputs & Progress)
You can build complex forms and progress states directly inside the notification.
### Text Input & Dropdowns
```lua
toast:input("admin_note", "Add a note to this operation...", "Admin Note")
    :selection("cleanup_method", {
        { id = "zip", content = "Archive to ZIP (Slow)" },
        { id = "delete", content = "Delete Permanently (Fast)" }
    }, "zip", "Select cleanup method:")
```
> Note: Values typed or selected by the user are returned in the args table of the "activated" event.
### Progress Bar
The `:progress()` method adds a native Windows progress bar that can be updated dynamically without re-sending the entire notification.
### `:progress(status, value, display, title)`
- `status` (string): Status text displayed below the bar (e.g., "Downloading...").
- `value` (number/string): The fill value of the bar.
  - Use a `number` between `0.0` and `1.0`.
  - Use the string `"indeterminate"` for a continuous loading animation (marquee).
- `display` (string, optional): A custom string shown over the default percentage (e.g., "45 of 100MB").
- `title` (string, optional): A secondary title specific to the progress section.
```lua
-- Initial Progress Bar example
toast:progress("Analyzing files...", 0.45, "45%", "Disk Cleanup")
```
### Updating Progress
To update a notification already on screen, call the `:update()` method on the `same instance` that was fired. This sends only the new data to Windows, ensuring a smooth transition without UI flickering.
### `:update(value, status, display, title)`
- `Returns:` boolean (success, or false if the toast was already dismissed).
```lua
-- Example update loop
for i = 1, 100 do
    local p = i / 100
    progress_toast:update(p, "Processing...", math.floor(p*100) .. "%")
    sleep(1) -- some sleep function
end
```
> [!WARNING]
## Unique Identification (Tag & ID)
By default, every call to `:fire()` creates a new independent notification. If you want to `replace` an existing toast or use the `:update()` method, you *`MUST`* define a `:tag()`
- `With a Tag:` If you fire a toast with `:tag("my-unique-id")`, any subsequent toast fired with the same Tag will silently replace the previous one in the Action Center.
- `Without a Tag:` The system generates a random unique ID. In this case, calling `:fire()` multiple times will "stack" different notifications on the user's screen.

### Progress Behavior Summary
| Scenario | `value` behavior|
|----------|-----------------|
| 0.0 to 1.0 | Progress bar filled proportionally.|
| "indeterminate" | "Loading" style bar (cyclic animation).|
| nil / Update | Keeps the previous value if not specified in the update call.|
## Important Notes on Actions
- Limit: Windows restricts notifications to a maximum of 5 actions. Adding more will trigger an error.
- Path Resolution: Just like the main icon, image paths in opts.image are automatically resolved to absolute paths by Lunalify.
- Event Handling: When an action is clicked, the target string is passed to your listen function inside the event.args field.


### Full Lifecycle Example
```lua
-- 1. Create and fire the initial progress toast
local ntf = lunalify.toast.create("Backup", "Starting data copy...")
    :tag("backup-job-01") -- Essential to allow future updates/replacement
    :progress("Preparing...", 0)
    :fire()

-- 2. Update the progress later
ntf:update(0.5, "Copying files...", "50%")

-- 3. Finalize by replacing the progress bar with a success message
lunalify.toast.create("Backup Complete", "All files were saved successfully.")
    :tag("backup-job-01") -- Same tag replaces the progress bar with this final result
    :fire()
```

---


## The Fire Executor (Advanced)
The `:fire()` method optionally accepts an `executor` function, which acts as a high-level middleware. This allows you to intercept the final notification data and control exactly `when` or `if` the dispatch should occur.

### How it Works
When you provide a function to` :fire()`, *Lunalify* delegates the execution flow to you. The executor receives four arguments: `title`, `message`, `opts`, and the internal `dispatch` function.
- *Scheduling Control:* You can delay the notification using sleep functions or external timers without blocking the main Lua execution thread.
- *Data Interception:* Useful for last-minute logging or validating dynamic conditions before the notification is sent to the operating system.
- *Manual Execution:* The notification will *only* be sent if you explicitly call the `dispatch` function within your executor.

> [!IMPORTANT]
The dispatch function handles the internal state and returns the id and error object, allowing you to process the final result directly within your custom executor logic.

```lua
lunalify.toast.create("Security", "Login attempt detected!")
    :fire(function(title, message, opts, dispatch)
        -- Example: Wait 5 seconds before firing
        print("Scheduling notification: " .. title)
        sleep(5) -- some sleep function
        local id, err = dispatch(title, message, opts)
        
        if id then
            print("Notification sent with Id: " .. id)
        end
    end)
```

## Event Handling
You can bind logic directly to a specific Toast using `:on()`. This allows you to handle user interactions or system events for that specific notification.
### `:on(event, callback)`
- `event` (string): The event to listen for ("activated", "dismissed", or "failed").
- `callback` (function): The function to execute when the event occurs.

1. The `activated` Event
Triggered when the user clicks the notification body or an interaction button.
- `args` (table): Contains the `action` Id and any input values (text or selections).
2. The `dismissed` Event
Triggered when the notification is removed from the screen.
- `reason` (string): A literal string indicating why the toast was dismissed.

| Reason | Description |
| -------| ------------|
| "UserCanceled" |  "The user explicitly closed the toast (clicked the 'X' or swiped it away)."
| "TimedOut" | "The toast expired and was moved to the Action Center (or disappeared).'
| "ApplicationHidden" | The notification was programmatically removed by your application.

```lua
toast:on("activated", function(args)
    -- args.action contains the Id of the clicked button
    if args.action == "exec_cleanup" then
        local note = args.admin_note
        print("Cleaning up with note: " .. note)
    end
end)
:on("dismissed", function(reason)
    if reason == "TimedOut" then
        print("Notification ignored: moved to Action Center.")
    elseif reason == "UserCanceled" then
        print("Notification closed manually by the user.")
    end
end)
```
3. The failed Event
Triggered if the system fails to display the notification.
- `error` (string): A description of the failure.
```lua 
toast:on("failed", function(err)
    print("Toast error: " .. err)
end)
```

> [!TIP]
`Automatic Cleanup:` To prevent memory leaks, Lunalify automatically clears the internal event registry for a specific notification once a `"dismissed"` or `"failed"` event is received. You do not need to manually unbind these listeners.

## How it interacts with `kit.setup()`
When you call `:fire()`, Lunalify automatically merges the **Global Settings** (from `lunalify.kit.setup`) with the Instance Settings (from the builder). Local methods always take precedence over global settings.

## Using Constants (Highly Recommended)
To avoid typos and ensure compatibility with Windows string requirements, Lunalify provides a built-in CONSTANTS table. Using these is safer than passing arbitrary strings.

### Accessing Constants
```lua
local LNFC = lunalify.CONSTANTS

local SOUNDS = LNFC.SOUNDS
local SCENARIOS = LNFC.SCENARIO
local DURATIONS = LNFC.DURATION

lunalify.toast.create("Update", "New version available")
    :duration(LNFC.DURATION.Long)
    :scenario(LNFC.SCENARIO.Reminder)
    :fire()
```

## Handling Failures
Most methods in the builder return the instance itself to allow chaining. However, `:fire(`) and `:update()` return specific status values. For a detailed guide on how to catch and handle errors, see [Error Handling Guide](./error-handling.md).
