--- Script module
-- @module el.script
local M = {}

--- Run a known script
-- @function exec
-- @string path The script to run
-- @tparam table env The environment to use or _ENV
-- @tparam any ... Arguments passed to script
-- @treturn any Return value from script or no value
-- @usage element.script ('scriptname')
function M.exec (path, env, ...)
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
