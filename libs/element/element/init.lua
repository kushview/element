--- Element main module
-- @module element

M = {}

local world = _G['element.world']
if not world then return error ("world not loaded") end

--- Get the world
-- @function world
-- @treturn element.World The global world instance
function M.world() return world end

--- Run a known script
-- @function script
-- @string path The script to run
-- @tparam table env The environment to use or _ENV
-- @tparam any ... Arguments passed to script
-- @treturn any Return value from script or no value
-- @usage element.script ('scriptname')
function M.script (path, env, ...)
    local file, e1 = package.searchpath (path, package.spath)
    if file then
        local fn, e2 = loadfile (file, 'bt', env or _ENV)
        if type(fn) == 'function' then
            return fn(...)
        end
        return e2
    end
    return e1
end

return M
