# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.3.0] - 2026-04-29
### Added
- `Async Event Polling:` Introduced `kit.update(callback)` utilizing Windows Overlapped I/O, allowing non-blocking event processing within custom application loops (Games, Neovim, GUI).
- `Global Event Monitoring:` Added `kit.watch(event, callback)` to allow persistent, global interception of specific event types across all notifications.
- `Native Sleep Utility:` Implemented `kit.sleep(ms)` via a native C++ binding to the Windows `Sleep` API, enabling high-efficiency loop throttling with near-zero CPU usage.

### Changed
- `Bridge Safety:` Wrapped internal event dispatching in `pcall` to ensure that faulty user callbacks do not crash the main event loop.


## [0.2.0] - 2026-04-28
### Added
- `Packet-Based Protocol:` Introduced `PacketHeader` with a Magic Number `(0x4C4E464C)` to ensure data integrity and protocol alignment between the Bridge and the Daemon.
- `IPC Synchronization:` Added an Acknowledge (ACK) mechanism via Pipe to prevent race conditions and ensure the Lua process only continues after the Daemon consumes the payload.
- `Global Context Management:` Implemented a C++ Singleton Context to manage persistent Pipe handles (`Command` and `Event` pipes) throughout the application lifecycle.

### Changed
- `Bridge Refactoring:` Migrated from raw string transmission to structured binary packets.
- `Client-Side XML Building:` Moved Toast XML generation from the Daemon to the Client (Bridge) to reduce Orchestrator overhead and improve UI responsiveness.
- `Pipe Communication Flow:` Refined the pipe opening logic to support persistent connections via the Context manager, reducing syscall overhead.

### Fixed
`Pipe Desync:` Resolved an issue where the Daemon read memory garbage due to misaligned payload sizes and lack of headers.

### Removed
`Redundant Disconnects:` Removed the requirement to force-close handles after every operation, now handled intelligently by the `Context`.


## [0.1.0] - 2026-04-24
### Added
- `Initial Release:`First functional release of the Lunalify.
- `Fluent Lua API:` Introduction of the `toast:create()` builder with method chaining support `(:title(), :text(), :progress())`.
- `Basic IPC:` Initial Named Pipe implementation for cross-process communication.
- `Event Callbacks:` Support for `activated`, `dismissed`, and `failed` toast events via `kit.listen`
