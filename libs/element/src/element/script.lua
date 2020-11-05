--- Script module
-- @module element.script

local M = {
    entries = {}
}

--- Runs a script
-- @string path
-- @return Error code or error
function M.run (path)
    if type(path) ~= 'string' then return error("cannot run:" .. type(path)) end
    local s = _G['element.C.World']:session()
    
    path = s:resolve (path)
    s:execute (code);
end

return M
