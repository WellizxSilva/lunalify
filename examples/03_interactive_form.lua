local lunalify = require("lunalify")
local LNFC = lunalify.CONSTANTS

lunalify.kit.setup({ app_id = "Lunalify.Example.Interactive" })

lunalify.toast.create("Security Alert", "A suspicious login was detected.")
-- Add a dropdown selection
    :selection("action_type", {
        { id = "block",  content = "Block IP Address" },
        { id = "ignore", content = "Ignore this time" },
        { id = "report", content = "Report to Admin" }
    }, "block", "Recommended Action:")

-- Add a text input for a reason/note
    :input("admin_note", "Write a brief reason...", "Justification")

-- Add styled buttons (Windows 11 Success/Critical colors)
    :action("Confirm", "submit_security", {
        style = "Success",
        image = "./assets/check.png"
    })
    :action("Cancel", "cancel_event", {
        style = "Critical"
    })

-- Handle the complex result
    :on("activated", function(args)
        if args.action == "submit_security" then
            print("Action Selected: " .. args.action_type)
            print("Admin Note: " .. args.admin_note)
        else
            print("Action cancelled by user.")
        end
    end)
    :fire()

print("Waiting for form submission...")
lunalify.kit.listen()
