local lunalify = require("lunalify")
local LNFC = lunalify.CONSTANTS

lunalify.kit.setup({ app_id = "Lunalify.Example.Events" })

print("--- Event Handling Demonstration ---")

local toast = lunalify.toast.create("Event Tester", "Interact with me to see the logs below.")
    :action("Click Me", "btn_primary")
    :action("Quick Reply", "btn_reply")
    :input("user_input", "Type something here...")

    -- Local 'Activated' Handler (Captures clicks and data)
    :on("activated", function(args)
        print("\n[EVENT: ACTIVATED]")
        print("Action ID: " .. (args.action or "Body Click"))
        if args.user_input then
            print("Input Value: " .. args.user_input)
        end
    end)

    -- Local 'Dismissed' Handler (Captures how the toast closed)
    :on("dismissed", function(reason)
        print("\n[EVENT: DISMISSED]")
        print("Reason: " .. reason)
        -- Reasons: UserCanceled (X or swipe), TimedOut (moved to center), ApplicationHidden
    end)

    -- Local 'Failed' Handler
    :on("failed", function(err)
        print("\n[EVENT: FAILED]")
        print("Error: " .. err)
    end)
    :fire()

print("Notification fired. The script is now waiting for Windows signals...")

-- The Global Event Loop
-- kit.listen() is blocking. It keeps the Lua process alive to receive IPC callbacks.
lunalify.kit.listen(function(ev)
    -- Optional: Global interceptor.
    -- Return 'false' here if you want to break the loop programmatically.
    if ev.event == "dismissed" and ev.reason == "UserCanceled" then
        print("User closed the toast. Exiting loop...")
        return false
    end
end)

print("Loop finished. Cleanup complete.")
