--- Widget module
-- @module element.widget

local Widget = {}
local M = {}

local object    = require ('element.object')
local ui        = require ('element.ui')

local Widget_getters = {
    name = function (self)
        if self.impl then return self.impl:getName()
        else return "" end
    end,

    visible = function (self)
        if self.impl then return self.impl:isVisible()
        else return false end
    end,

    width = function (self)
        if self.impl then return self.impl:getWidth()
        else return 0 end
    end,

    height = function (self)
        if self.impl then return self.impl:getHeight()
        else return 0 end
    end
}

local Widget_setters = {
    name = function (self, value)
        if type(value) == 'string' then
            self.impl:setName (value)
        end
    end,

    visible = function (self, value)
        if type(value) == 'boolean' then
            print ("set visible: " .. tostring (value))
            self.impl:setVisible (value)
        end
    end
}

function Widget:init()
end

function Widget:addtodesktop()
    self.impl:addToDesktop()
end

function Widget:setsize (w, h)
    self.impl:setSize (w, h)
end

function Widget:resized()
end

function Widget:paint (g)
end

function Widget:repaint()
    self.impl:repaint()
end

function M.derive (base, wrap, get, set)
    local Derived = {}
    base = base or Widget
    wrap = wrap or ui.ComponentWrapper.create
    for k, v in pairs (base) do
        Derived[k] = v
    end

    Derived.SuperType = base

    function Derived.new ()
        local g = {}
        local s = {}
        for k, f in pairs (Widget_getters)  do g[k] = f end
        for k, f in pairs (Widget_setters)  do s[k] = f end

        if type (get) == 'table' then
            for k, f in pairs (get)         do g[k] = f end
        end
        if type (set) == 'table' then
            for k, f in pairs (set)         do s[k] = f end
        end

        local self = object.instance (Derived, {}, g, s)
        self.impl = wrap (self)

        local inits = {}
        local T = Derived
        while T ~= nil do
            table.insert (inits, 1, T.init)
            T = T.SuperType
        end

        for _, init in ipairs (inits) do
            if init then init (self) end
        end

        return self
    end

    return Derived
end

return M
