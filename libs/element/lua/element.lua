--- Element main module
-- @module element
local M = {}

local world = _G['element.world']
if not world then return error ("world not loaded") end

--- Get the world
-- @function world
-- @treturn element.World The global world instance
function M.world() return world end

function M.version()
    return "1.0.0"
end

return M
