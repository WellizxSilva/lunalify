local Errors = require("lunalify.errors")
local Helpers = {}
local Path = {}
Helpers.Path = Path

--- @param path string The path to resolve
--- @param strict boolean? Whether to throw an error if the path does not exist
function Path.resolve_path(path, strict)
    if strict == nil then strict = true end            -- default to true
    if not path or path == "" then return "", true end -- Empty is valid

    local is_absolute = path:match("^%a:[\\/]")
    local full_path = ""

    if is_absolute then
        full_path = path:gsub("/", "\\")
    else
        local handle = io.popen("cd")
        if not handle then
            Errors.throw(3, "Failed to execute 'cd'", 2)
            return "", false
        end
        local currentDir = handle:read("*all"):gsub("[\n\r]", "")
        handle:close()
        local clean_path = path:gsub("^%.[\\/]", "")
        full_path = (currentDir .. "\\" .. clean_path):gsub("/", "\\")
    end
    -- Normalize the path by removing '\\' and '\\.' sequences Ex:(folder\\.\file.txt -> folder\file.txt)
    full_path = full_path:gsub("\\%.\\", "\\"):gsub("\\+", "\\")

    local f = io.open(full_path, "r")
    if f then
        f:close()
        return full_path, true
    else
        if strict then
            Errors.throw(1, full_path, 2)
            return "", false       -- Return 'false' to indicate failure
        else
            return full_path, true --Return the 'full_path' even if it doesn't exist to allow windows to create it
        end
    end
end

function Helpers._parse_payload(payload)
    if type(payload) ~= "string" or payload == "" then
        return { action = "default" }
    end
    local data = {}
    -- Captures the main action (before the first |)
    data.action = payload:match("([^|]+)")

    for k, v in payload:gmatch("([^|%s=]+)=([^|]*)") do
        data[k] = v:match("^%s*(.-)%s*$") -- trim value
    end
    setmetatable(data, {
        __index = function() return "" end
    })

    return data
end

return Helpers
