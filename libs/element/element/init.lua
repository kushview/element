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
-- @treturn int Return code
-- @usage element.script ('scriptname')
function M.script (path, env, ...)
    local filename = '/home/mfisher/workspace/kushview/element/scripts/' .. path .. '.lua'
    local f, err = loadfile (filename, 't', env or _ENV)
    if type(f) == 'function' then
        return f(...)
    end
    return err
end

-- @usage element.script ('path/to/script')

return M
