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
    self.knob.style = Slider.ROTARY
    self.knob:settextboxstyle (Slider.TEXT_BOX_BELOW, true, 52, 26)
    self.knob:setrange (-90, 24, 0.01)
    self.knob:setvalue (volume:get(), false)
    self.knob.dragging  = false
    self.knob.dragstart = function() self.knob.dragging = true end
    self.knob.dragend   = function() self.knob.dragging = false end
    self.knob.valuechanged   = function()
        if not self.knob.dragging then return end
        volume:set (self.knob:value(), false)
    end

    volume.valuechanged = function()
        self.knob:setvalue (volume:get(), false)
    end

    self:resize (180, 170)
end

function Editor:paint (g)
    g:fillall (bgcolor)
    g:setcolor (fgcolor)
    g:drawtext ("AMP", 0, 0, self.width, 30)
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
    type    = 'DSPUI',
    editor  = instantiate
}
