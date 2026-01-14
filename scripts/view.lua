--- A new View script (experimental)
-- 
-- Script description.
-- 
-- @type View
-- @license GPL-3.0-or-later
-- @author Your Name

local object = require ('el.object')
local new = object.new

local function instantiate()
    local w = new ('el.View')
    w.paint = function (_, g)
        g:fillAll(0xff00ff00)
        g:setColor (0xffffffff)
        g:drawText ("Hello World", w:localBounds())
    end
    return w
end

local function handleGraphChange(_, graph)
end

return {
    type = "View",
    instantiate = instantiate,
    graph_changed = handleGraphChange
}

-- SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
-- SPDX-License-Identifier: GPL-3.0-or-later
