--- A GUI Window.
-- Inherits from `el.Widget`
-- @classmod el.Window
-- @pragma nostrip

local object = require ('kv.object')
local ui     = require ('element.ui')
local Widget = require ('el.Widget')

local Window = object (Widget, {
    --- Window visibility (bool).
    -- Setting true places an OS-level window on the desktop.
    -- @class field
    -- @name Widget.desktop
    -- @within Attributes
    desktop = {
        get = function (self) return self.impl:isOnDesktop() end,
        set = function (self, value)
            if self.impl then
                if value then self.impl:addToDesktop()
                else self.impl:removeFromDesktop() end
            end
        end
    }
})

function Window:init()
    Widget.init (self, ui.WindowWrapper.create (self))
end

--- Set the widget to display.
-- @tparam el.Widget w Displayed widget
function Window:setwidget (w)
    if not w then return end
    self.impl:setContentOwned (w.impl, true)
end

return Window
