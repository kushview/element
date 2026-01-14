--- List commands registered in the command manager.
-- @script commands
-- @type anonymous
-- @license GPLv3
-- @author Michael Fisher
-- @usage
-- > script.exec ('commands')
-- SHOW_ABOUT = 256
-- SHOW_ALL_PLUGIN_WINDOWS = 265
-- SHOW_SESSION_CONFIG = 260
-- RECENTS_CLEAR = 4096
-- GRAPH_OPEN = 1793
-- GRAPH_SAVE_AS = 1795
-- UNDO = 4104
-- ...

local CMD   = require ('el.CommandManager')
local strings  = require ('el.strings')
local format = select (1, ...) or 'constants'

if not strings.isslug (format) then
    error ("Invalid format: " .. tostring (format))
end

local list = {
    -- List as constants used with the el.command module
    constants = function (cmds)
        for k,v in pairs (cmds) do
            local out = string.upper (strings.tosnake (k))
            out = out .. " = " .. tostring(v)
            print (out)
        end
    end
}

local cmds = {}

-- add standard commands
for _, cmd in ipairs (CMD.standard()) do
    local str = CMD.tostring (cmd)
    if string.len(str) > 0 then
        cmds[str] = cmd        
    end
end

if type(list[format]) == 'function' then
    list[format] (cmds)
end

-- SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
-- SPDX-License-Identifier: GPL-3.0-or-later
