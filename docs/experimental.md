# Experimental Features
This document tracks features currently under architectural review or in early-stage development. These features are **not** part of the stable Lunalify API and are subject to breaking changes.

---
# Non-Blocking Event Polling
The current `kit.listen()` is a blocking (synchronous) call, which is ideal for CLI tools but problematic for GUI applications that must maintain a consistent frame rate (60+ FPS).
### The Challenge: Named Pipe Synchronization
Windows Named Pipes in "Byte Mode" typically block the thread until data is available. Implementing a naive `poll()` function in a high-frequency loop presents several risks:
  - **Handle Churn:** Repeatedly opening/closing pipe handles every frame creates unnecessary Kernel overhead.
  - **Race Conditions:** If the Daemon attempts to write an event exactly when the Parent process is "between polls", the message might be delayed or the pipe might report a "Busy" state.
  - **I/O Starvation:** Constant polling can starve the CPU if not implemented with proper OS-level signals.

## Proposed API (Draft)
1. `lunalify.kit.poll()`
A non-blocking check that returns `nil` if no event is in the pipe, or the `event` table if data is ready.
```lua
-- Example of proposed GUI integration
function Application_Framework.update(dt)
    local ev = lunalify.kit.poll()
    if ev then
        print("Async event received: " .. ev.event)
    end
end
```
2. `lunalify.kit.observe()` (Corountine Wrapper)
A helper that wraps the polling logic into a Lua Coroutine, allowing developers to `yield` until a notification is interacted with, without freezing the rest of the application.
---
