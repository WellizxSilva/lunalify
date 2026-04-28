local lunalify = require("lunalify")

lunalify.kit.setup({ app_id = "Lunalify.Example.Progress" })

-- We use a tag to ensure we update the SAME toast instead of creating 100 new ones
local my_tag = "task-progress-001"

local toast = lunalify.toast.create("System Update", "Downloading package...")
    :tag(my_tag)
    :progress("Initializing...", 0, "0%", "Update Manager")
local ok, err = toast:fire()
if not ok and err then
    print('An error occurred: ' .. err.message)
end

-- Simulate a background task
for i = 1, 100 do
    local percent = i / 100
    local status = (i < 100) and "Downloading..." or "Download Complete!"

    -- Update the progress bar smoothly
    toast:update(percent, status, i .. "%")

    -- Small delay to simulate work
    os.execute("timeout /t 1 > nul") -- (Use an appropriate 'sleep' function, ex: socket.sleep())
end

print("Update finished. Check your Action Center.")
