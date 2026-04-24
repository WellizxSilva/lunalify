# 🌙 Lunalify

---
Lunalify is a lightweight high-performance Lua module for sending native Windows 11 Toast notifications. Using a unique a C++ Daemon architecture, it ensures your notifications are interactive and persistent.

## Key Features
- **Modern Windows 11 UI:** Support for icons, hero images, and up to 5 interactive buttons.
- **Native Styling:** Rich button actions with custom icons and native Windows semantic colors (Success/Critical).
- **Persistent Daemon:** Captures clicks and dismissals even if your main script is busy.
- **Zero CPU Idle:** Uses Windows Named Pipes and efficient waiting (no busy-loops).
- **Crash Protection:** Employs Windows Job Objects to ensure the Daemon dies if your app crashes.


# System Requirement
- **OS:** Windows 11 (Build 22000+).
- **Lua:** 5.1 (JIT), 5.3, or 5.4.
- **Compiler** MinGW-w64 (GCC 13+) or MSVC (VS 2022) with C++20.
- **Dependencies::** Windows Runtime (WinRT) support.

---

# Installation
### Option A: Via LuaRocks (Recommended)
```sh
luarocks install lunalify
```
> [!IMPORTANT]
> After installation, ensure your `LUA_PATH` and `LUA_CPATH` are correctly configured in your System Environment Variables so Lua can find the installed rocks.
### Option B: Portable Bundle (Direct Download)
Ideal for quick projects or embedding.
1. Download the `lunalify-vX.X.X-win64.zip` from [Latest Releases](https://github.com/WellizxSilva/lunalify/releases).
2. Extract the files into your project. Suggested structure:

```
your_project/
├── lunalify/      # Lunalify files
│   └── lunalify/
│   │   ├── init.lua
│   └── bin/
│       └── lunalify_core.dll # Native binary
└── src # your structure
```

## Environment Setup (Important)
1. Setting up Paths
- To run Lunalify from any terminal, you must add the LuaRocks paths to your Windows Environment Variables. Run `luarocks path` to see your specific paths.
2. IntelliSense
- To get auto-complete and type definitions config your Editor/IDE environments correctly

## Internal Loading Mechanism
Lunalify uses a **Hybrid Loading System** to ensure flexibility:

1. LuaRocks Context: When installed via LuaRocks, Lunalify is loaded via the standard `require("lunalify")`. The `init.lua` automatically looks for lunalify_core in the system's `CPATH` .
2. Manual/Local Loading: If you are not using LuaRocks, Lunalify features a fallback mechanism. It will attempt to find the native binary `(lunalify_core.dll)` in:
 - `./bin/lunalify_core.dll`
 - Path relative to the script execution.

 ---

# Quick Start
```lua
local lunalify = require("lunalify")

-- 1. Setup (Registers your App in Windows Start Menu)
local APP_ID = 'MyProject.Lunalify.Notifier'
local APP_NAME = 'Lunalify Assistant'
local success, err = lunalify.kit.setup({
    app_id = APP_ID, -- Pass an empty string or nil to use the stable system default.
    app_name = APP_NAME,
    logging = { enabled = true, to_console = false }
})

if not success and err then
    print("Error registering " .. tostring(err))
end

-- 2. Construct the Toast
local toast = lunalify.toast.create("System Optimized", "Logs archived and space recovered.")
    :icon("./assets/success_icon.png")
    :action("Great!", "btn_great", { style = "Success", image = "./assets/check.png" })
    :action("Undo", "btn_undo", { style = "Critical", image = "./assets/cancel.png" })
    :on("activated", function(args)
        print("User interacted! Action:", args.action)
    end)
    :on("dismissed", function(reason)
        print("Toast was dismissed. Reason:", reason)
    end)

-- 3. Fire and Listen
toast:fire()

-- listen() is blocking and starts the event loop
lunalify.kit.listen(function(ev)
    if ev.event == "dismissed" then return false end -- Stop loop when dismissed
end)

-- 4. Graceful Shutdown
lunalify.kit.shutdown()
```

# App Identity & Custom IDs
> [!CAUTION]
> **Important for Windows 11 Compatibility**
By default, *Lunalify* is designed to be "plug-and-play". However, Windows 11 enforces strict rules on how apps send notifications:
1. **Default Identity:** If no specific configuration is required, *Lunalify* can use a pre-validated System AppId to ensure notifications work instantly.
2. **Custom IDs:** When you provide a `app_id` in `kit.setup()`, *Lunalify* attempts to register a new shortcut in your Start Menu with that Id.
  - **Indexing Delay:** Windows may take several seconds to "trust" and index a newly created AppId.
  - **Validation:** If the AppId is not correctly indexed by the Windows Shell, you may encounter an `Element Not Found error.`
3. **Recommendation:** For development, the default settings are fine. If you need a custom Id, `test your AppId thoroughly`. Ensure your app's installer creates the Start Menu shortcut correctly if you notice any registration delays (or `Element Not Found Error`).

---

## Critical Notes
To ensure Lunalify operates reliably, please note:
1. Daemon Lifecycle
Lunalify spawns a hidden lua.exe (The Daemon) to manage the WinRT bridge.
  - **Always call** `lunalify.kit.shutdown()` before your app exits to close the Named Pipes and kill the Daemon gracefully.
  - **Job Objects**: If your app crashes, Windows will kill the Daemon automatically. No zombie processes.

2. Image Path Resolution
> [!TIP]
`Use Absolute Paths:` Windows WinRT requires Absolute Paths for images. While `Lunalify` tries to resolve relative paths `(e.g., ./icon.png)`, using full absolute paths is highly recommended to avoid silent failures where notifications appear without images.


3. Blocking vs Non-Blocking
> [!CAUTION]
**`kit.listen()`** is blocking. It's perfect for CLI tools or scripts that wait for a user response.
For non-blocking event loop check the [Experimental Documentation](./docs/experimental.md) for more information

---
 
# 📚 Documentation
To explore the full power of Lunalify, please check our detailed documentation:
- [API Reference: Kit](./docs/kit-reference.md) - Everything about setup, lifecycle, and event loops.
- [Toast Construction](./docs/toast-reference.md) - How to build complex notifications with actions and images.
- [Error Handling Guide](./docs/error-handling.md) - Detailed guide on how to catch and handle errors.
- [Architecture & IPC](./docs/architecture.md) - Deep dive into the Daemon, Pipes, and Job Objects.
- [Experimental](./docs/experimental.md) - Features that are currently in development or under architectural review.

## Contribuiting
Found a bug or have a feature request? Please [open a new issue](https://github.com/WellizxSilva/lunalify/issues). Contributions are welcome!


# License
Distributed under the *MIT* License. See [LICENSE](./LICENSE) for more information.

---
