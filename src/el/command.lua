--- UI Commands.
-- Support library for working with Element UI commands.
-- @module el.command

local Commands = require ('el.Commands')
local Context  = require ('el.Context')

local M = {}

local function instance()
    local g = Context.instance()
    return g and g:commands() or nil
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
        local m = instance()
        assert (m ~= nil, "nil el.Commands")
        return m:invokeDirectly (cmd, async or false)
    end

    return false
end

--- Create a closure for a command.
--
-- Returns a function which invokes the command async or not. By default the
-- command is not invoked async
-- @int cmd Command ID
-- @bool async Whether to invoke async or not
-- @return function Function which invokes the command
-- @usage
-- local show_about = command.closure (command.SHOW_ABOUT, true)
-- show_about()
function M.closure (cmd, async)
    assert (type (cmd) == 'number', "command must be a number")

    local c = math.tointeger (cmd)
    local a = true
    if type(async) == 'boolean' then
        a = async
    elseif type(async) == 'number' then
        a = async ~= 0.0
    elseif type(async) == 'string' then
        a = string.len (async) > 0
    elseif type(async) == 'nil' then
        a = true
    end

    return function() return M.invoke (c, a) end
end

-- Define standard commands as constants. 
-- e.g. Commands::showAbout in C++ becomes command.SHOW_ABOUT
for _,cmd in ipairs (Commands.standard()) do
    local strings = require ('el.strings')
    local s = Commands.toString (cmd)
    if string.len(s) > 0 and strings.valid(s) then
        local k = strings.tosnake (s)
        M[string.upper (k)] = cmd
    end
end

return M

-- SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
-- SPDX-License-Identifier: GPL-3.0-or-later
