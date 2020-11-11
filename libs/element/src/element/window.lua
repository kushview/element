--- A GUI Window
-- @module element.window
local M = {}

local ui     = require ('element.ui')
local widget = require ('element.widget')

--- Base window type
-- @type Window
local Window = widget.derive (nil, ui.WindowWrapper.create)

--- Set the displayed widget
-- @function setwidget
function Window:setwidget (w)
    if not w then return end
    self.impl:setContentOwned (w.impl, true)
end

--- Present the window to the desktop
-- @function present
function Window:present()
    self.impl:addToDesktop()
end

--- Creates a new window
-- @function new
function M.new (w)
    local win = Window.new()
    if w then win:setwidget (w) end
    return win
end

return M
