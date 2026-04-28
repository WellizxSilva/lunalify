local lunalify = require("lunalify")
local LNFC = lunalify.CONSTANTS

-- Initialize the library and register your App ID with Windows
-- This creates the necessary Start Menu shortcut for notifications to work.
local ok, err = lunalify.kit.setup({
    app_id = "Lunalify.Example.Basic",
    app_name = "Lunalify Basic Example",
    icon_path = "./assets/logo.ico", -- Path to your app icon
    logging = {
        enabled = true,
        to_console = true,
        level = 1 -- Info level
    }
})

if not ok and err then
    print("Setup failed: " .. err.message)
    os.exit(1)
end

-- Build and Fire a simple notification
lunalify.toast.create("Hello World", "This is a native Windows 11 notification!")
    :icon("./assets/info.png")
    :duration(LNFC.DURATION.Short)
    :on("activated", function()
        print("The user clicked the notification body!")
    end)
    :fire()

-- Start the event loop to catch the 'activated' callback
print("Waiting for interaction...")
lunalify.kit.listen(function(ev)
    if ev.event == "dismissed" then return false end -- Stop loop when dismissed
end)
