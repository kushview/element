--- Widget module
-- @module element.widget

local Widget = {}
local M = {}

local ui = require ('element.ui')

local function defaultwrap (w)
    local wrapper = ui.ComponentWrapper.new()
    wrapper:wrap (w)
    return wrapper
end

function Widget:init()
end

function Widget:addtodesktop()
    self.impl:addToDesktop()
end

function Widget:setsize (w, h)
    self.impl:setSize (w, h)
end

function Widget:setvisible (v)
    self.impl:setvisible (v)
end

function Widget:resized()
end

function Widget:paint (g)
end

function M.derive (base)
    local Derived = {}
    base = base or Widget
    local wrap = defaultwrap
    for k, v in pairs (base) do
        Derived[k] = v
    end

    Derived.SuperType = base

    function Derived.new()
        local self = setmetatable ({}, { __index = Derived })
        self.impl = wrap (self)
        
        local inits = {}
        local T = Derived
        while T ~= nil do
            table.insert (inits, 1, T.init)
            T = T.SuperType
        end

        for _, init in ipairs (inits) do
            if init then init() end
        end

        return self
    end

    return Derived
end

return M
