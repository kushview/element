--- Commands.
-- @module el.command

local CommandManager    = require ('el.CommandManager')
local Globals           = require ('el.Globals')

local M = {}

--- Returns the global el.CommandManager
function M.manager()
    local g = Globals.instance()
    return g and g:command_manager() or nil
end

--- Invoke a command.
-- @int cmd Command ID
-- @bool async Invoke async or not (default false)
function M.invoke (cmd, async)
    if type(cmd) == 'number' then
        cmd = math.tointeger (cmd)
    else
        cmd = 0
    end

    if cmd > 0 then
        local m = M.manager()
        assert (m ~= nil, "nil el.CommandManager")
        return m:invoke_directly (cmd, async or false)
    end

    return false
end

--- Create a closure for a command.
-- Returns a function which invokes the command async or not. By default the
-- command is not invoked async
-- @int cmd Command ID
-- @usage
-- local show_about = command.closure (command.SHOW_ABOUT)
-- local async = true
-- show_about (async)
function M.closure (cmd)
    assert (type (cmd) == 'number', "command must be a number")
    local c = math.tointeger (cmd)
    return function (async)
        return M.invoke (c, async or false)
    end
end

-- Define standard commands as constants. e.g. Commands::showAbout in C++ 
-- becomes command.SHOW_ABOUT
for _,cmd in ipairs (CommandManager.standard()) do
    local slug = require ('kv.slug')
    local s = CommandManager.tostring (cmd)
    if string.len(s) > 0 and slug.valid(s) then
        local k = slug.tosnake (s)
        M[string.upper (k)] = cmd
    end
end

return M
