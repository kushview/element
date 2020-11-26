--- A GUI Widget
-- @classmod el.Widget
-- @pragma nostrip

local ui        = require ('element.ui')
local object    = require ('kv.object')

local Widget = object {
    --- Widget name (string).
    -- @class field
    -- @name Widget.name
    -- @within Attributes
    name = {
        get = function (self) return self.impl:get_name() end,
        set = function (self, value) self.impl:set_name (value) end
    },

    --- Widget visibility (bool).
    -- @class field
    -- @name Widget.visible
    -- @within Attributes
    visible = {
        get = function (self) return self.impl:is_visible() end,
        set = function (self, value) self.impl:set_visible (value) end
    },

    --- Widget bounding box (table).
    -- The coords returned is relative to the top-left of the widget's parent.
    -- @class field
    -- @name Widget.bounds
    -- @within Attributes
    -- @usage
    -- widget.bounds = {
    --     x      = 0, 
    --     y      = 0,
    --     width  = 100,
    --     height = 200
    -- }
    bounds = {
        get = function (self) return self.impl:get_bounds() end,
        set = function (self, value)
            if 'table' == type(value) then self:set_bounds (value) end
        end
    },

    --- Widget width (integer).
    -- @class field
    -- @name Widget.width
    -- @within Attributes
    width = {
        get = function (self) return self.impl:get_width() end
    },

    --- Widget height (integer).
    -- @class field
    -- @name Widget.height
    -- @within Attributes
    height = {
        get = function (self) return self.impl:get_height() end
    }
}

--- Initialize the widget.
-- Override this to customize your widget
function Widget:init (impl)
    self.impl = impl or ui.ComponentWrapper (self)
end

--- Add a child widget.
-- @tparam el.Widget widget Widget to add
-- @int zorder Z-order (default -1)
function Widget:add (widget, zorder)
    zorder = zorder or -1
    self.impl:add (widget, zorder)
    return widget
end

--- Change this widget's size
-- @int width   New width
-- @int height  New height
function Widget:resize (width, height)
    self.impl:resize (width, height)
end

--- Set the widget's bounds
function Widget:set_bounds (...)
    if select("#", ...) >= 4 then
        self.impl:setbounds (
            select (1, ...),
            select (2, ...),
            select (3, ...),
            select (4, ...)
        )
    else
        local r = select (1, ...)
        self.impl:set_bounds (r.x, r.y, r.width, r.height)
    end
end

--- Called when the widget changes in size.
-- Override this to layout child widget's
function Widget:resized()
end

--- Called when the widget needs drawn
-- @tparam juce.Graphics g Graphics context to draw with
function Widget:paint (g)
end

--- Repaint the entire widget
function Widget:repaint()
    self.impl:repaint()
end

function Widget:mouse_drag (ev) end
function Widget:mouse_down (ev) end
function Widget:mouse_up (ev) end

return Widget
