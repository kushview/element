--- Editor for `amp` DSP script.
-- @script ampui
-- @type DSPUI amp

local Widget        = require ('el.Widget')
local Slider        = require ('el.Slider')
local object        = require ('el.object')
local script        = require ('el.script')

local bgcolor       = 0xff545454
local fgcolor       = 0xffffffff

-- Derive a new widget type for the amp editor.
local Editor = object (Widget)

function Editor:init (ctx)
    -- Invoke the parent class' init method
    Widget.init (self)

    local volume = ctx.params [1]

    self.knob = self:add (object.new (Slider))
    self.knob.style = Slider.Rotary
    self.knob:setTextBoxStyle (Slider.TextBoxBelow, true, 52, 26)
    self.knob:setRange (-90, 24, 0.01)
    self.knob:setValue (volume:get(), false)
    self.knob.dragging  = false
    self.knob.dragStart = function() self.knob.dragging = true end
    self.knob.dragEnd   = function() self.knob.dragging = false end
    self.knob.changed   = function()
        if not self.knob.dragging then return end
        volume:set (self.knob:value(), false)
    end

    volume.changed = function()
        self.knob:setValue (volume:get(), false)
    end

    self:resize (180, 170)
end

function Editor:paint (g)
    g:fillAll (bgcolor)
    g:setColor (fgcolor)
    g:drawText ("AMP", 0, 0, self.width, 30)
end

function Editor:resized()
    local r = self:localBounds()
    self.knob:setBounds (r:reduced (20))
end

-- Editor factory function.
local function instantiate (ctx)
    return object.new (Editor, ctx)
end

return {
    type        = 'DSPUI',
    instantiate = instantiate
}

-- SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
-- SPDX-License-Identifier: GPL-3.0-or-later
