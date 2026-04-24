# 🏗️ Architecture & IPC Deep Dive

Lunalify is designed to be a high-performance bridge between Lua and the **Windows Runtime (WinRT)**. Unlike legacy solutions that spawn ephemeral PowerShell processes-leading to significant latency and "zombie" process overhead-Lunalify utilizes a **Persistent Daemon Architecture**.

## The Core Components

### 1. Parent Process (Lua Application)
This is your main application. By loading the `lunalify_core.dll`, it gains the ability to communicate with the Windows notification subsystem. It handles application logic, constructs Toast objects using the Fluent API, and dispatches them via the IPC bridge.

### 2. The Lunalify Daemon **(STA Bridge)**
Upon calling `lunalify.kit.setup()`, the library initializes a background worker process.
  - **STA Isolation:** *WinRT* requires a **Single-Threaded Apartment (STA)** to manage Toast life-cycles. By offloading this to a dedicated Daemon, your main Lua script remains non-blocking and highly responsive.
  - **Event Persistence:** The Daemon stays alive to capture user interactions (clicks, dismissals, input data) even if the parent process is performing heavy computations.

### 3. IPC: Named Pipes
`Lunalify` uses **Windows Named Pipes** for **Inter-Process Communication (IPC)**. This provides a low-latency, kernel-mode memory bridge between processes:
  - **Command Pipe:** A unidirectional flow where the Parent writes serialized XML payloads for the Daemon to process.
  - **Event Pipe:** A flow where the Daemon pushes real-time user interactions (button clicks, text input, expiration) back to the Parent.

### 4. Safety: Kernel Job Objects
One of the biggest issues with background processes is "orphaning" when the main app crashes, but the background process stays alive, consuming memory. 
  - **The Solution:** Lunalify assigns the Daemon to a **Windows Kernel Job Object** with the `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE` flag. 
  - **Zero Resource Leakage:** If the parent process terminates (whether via a graceful exit or a crash), the *Windows Kernel* automatically terminates the Daemon instantly. No manual cleanup required.

---

## Data Flow Lifecycle
The following diagram illustrates the lifecycle of a notification:

1. **Serialization `Toast:fire()`:** serializes the Lua object into a Windows-compliant XML schema.
2. **Dispatch:** The XML is sent through the **Command Pipe**. This is an asynchronous operation from the Lua perspective.
3. **`WinRT`**: The Daemon receives the command and tells the **Windows Action Center** to render the toast.
4. **`Event Loop`**: When the user interacts, Windows signals the `Daemon` via **COM/WinRT** events..
5. **Payload Parsing**: The Daemon packages the interaction data into a Lua-readable format and writes it to the *Event Pipe*.
5. **Execution `kit.listen()`**: Catches the event and triggers the specific callback defined in you `on("activated", ...)` handler

## Why This Architecture?

| Feature | Lunalify Daemon | Legacy(Powershell)|
|------|--------|--------------------------------------------|
| Dispatch Latency | < 15ms | > 500ms|
| Memory Footprint | Static & Minimal | Spikes per notification|
| Interaction | Bi-directional (Inputs/Events) | One-way (Fire and forget)
| Reliability | Job Object Protected | Risk of "zombie" processes

## Security Note
Named Pipes are created with security descriptors limited to the current user session, ensuring that notification data and event logs cannot be intercepted by other users or unauthorized processes on the same machine.
