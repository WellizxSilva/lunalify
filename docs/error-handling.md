# Error Handling in Lunalify

Lunalify uses a dual error-reporting system: **Synchronous** (returns from functions) and **Asynchronous** (event callbacks).

## 1. The Error Object
All errors follow this strict structure to allow for both programmatic handling and easy debugging:
`{ code: number, name: string, type: string, message: string }`
| Field      | Type | Description|
| :--------- | :------- | :--------|
| code       | number   | Unique numeric Id. e logic.|
| name       | string   | Semantic name (e.g., "IPC_TIMEOUT").|
| type   | string   | The error's origin: SYSTEM, IPC (Daemon), WINRT (Windows)|
| message    | string   | Detailed explanation of the error and potential resolution steps.|
## 2. Synchronous Errors
These occur immediately during the function call (e.g., invalid paths, Daemon offline, or Windows notifications globally disabled).

### Setup Errors (`kit.setup`)
Fails if the environment is not ready.
```lua
local ok, err = lunalify.kit.setup(config)
if not ok then 
    -- err is a Lunalify.Error table
    print(err.code, err.message) 
end
```
### Dispatch Errors `(:fire)`
Fails if the XML payload is malformed, Daemon is disconnected, etc.

```lua
local result, err = toast:fire()

if not result then
    print("Error Type: " .. err.type) -- e.g., "WINRT"
    print("Error Name: "     .. err.name)     -- e.g., "WINRT_DISABLED_BY_USER"
    print("Message: "        .. err.message)  -- e.g., "Disabled for user, toast blocked Id: <id> "
end
```


### 3. Asynchronous Errors `(:onError)`
Sometimes the delivery to the Daemon is successful, but Windows fails to render the toast later (e.g., an invalid XML structure).
```lua
toast:onError(function(err)
    print("Async failure detected!")
    print("Code:", err.code)       -- e.g., 302
    print("Name:", err.name)       -- e.g., "WINRT_INVALID_XML"
    print("Message:", err.message)
end)
```

## Error Constants

To avoid hardcoding numbers, always use the `lunalify.CONSTANTS.Errors` table for comparisons.

### Example
```lua
local ok, err = toast:fire()
if not ok and err.code == lunalify.CONSTANTS.Errors.WINRT_DISABLED_BY_USER then
    print("User disabled notifications")
end
```
