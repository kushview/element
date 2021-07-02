--- Editor for `examples.sysex` DSP script.
-- @script examples.sysexui
-- @kind DSPUI examples.sysex

local Widget        = require ('kv.Widget')
local TextButton    = require ('kv.TextButton')
local object        = require ('kv.object')
local new           = object.new
local script        = require ('el.script')

local bgcolor       = 0xff545454
local fgcolor       = 0xffffffff

local Editor        = object (Widget)

function Editor:init (ctx)
    Widget.init (self)
    self.ctx = ctx

    local led = self.ctx.params [1]
    self.button = self:add (new (TextButton))
    self.button.clicked = function (btn)
        if led:get() == 0 then
            led:set (1)
        else
            led:set (0)
        end
        self:stabilize()
    end

    self:resize (180, 120)
    self:stabilize()
end

function Editor:stabilize()
    local led = self.ctx.params [1]
    if led:get() == 0 then
        self.button:settogglestate (false, false)
        self.button.text = "Off"
    else
        self.button:settogglestate (true, false)
        self.button.text = "On"
    end
end

function Editor:paint (g)
    g:setcolor (bgcolor)
    g:fillall()
end

function Editor:resized()
    local r = self:localbounds()
    self.button:setbounds (r:reduced (20))
end

local function create_editor (ctx)
    return object.new (Editor, ctx)
end

return script.dspui {
    editor = create_editor
}
