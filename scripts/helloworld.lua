--- Show a custom widget in a window.
-- The return value is the displayed kv.DocumentWindow or nil
-- @script helloworld
-- @usage
-- local win = script.exec ('helloworld')

local object            = require ('kv.object')
local DocumentWindow    = require ('kv.DocumentWindow')
local TextButton        = require ('kv.TextButton')
local Widget            = require ('kv.Widget')
local new = object.new

local colors = {
    text        = 0xffffffff,
    background  = 0xff545454
}

local Label = object (Widget, {
    text = {
        set = function (self, value)
            rawset (self, '_text', value)
            self:repaint()
        end,
        get = function (self)
            return rawget (self, '_text') or ""
        end
    }
})

function Label:init()
    Widget.init (self)
    self.text = ""
    self:resize (100, 100)
end

function Label:paint (g)
    g:color (colors.text)
    g:drawtext (self.text, 0, 0, self.width, self.height)
end

local HelloWorld = object (Widget)
function HelloWorld:init()
    Widget.init (self)
    self.name = "HelloWorld"

    local label = new (Label)
    label.name = "DisplayLabel"
    label.text = "Hello world..."
    self.original_text = label.text
    self.label_height = 32
    self.label_width  = 120
    self.label = self:add (label)

    self.button = new (TextButton)
    self.button.text = "Quit"
    self.button.name = "TextButton"
    self.button:resize (90, 24)

    self:add (self.button, 9999)
    self:resize (640, 360)
end

function HelloWorld:resized()
    local r = self.localbounds:reduced (8)
    self.button.bounds = r:slice_bottom (30)
                          :slice_right (90)
    self.label.bounds  = r:reduced (20)
end

function HelloWorld:paint (g)
    g:fillall (colors.background)
end

function HelloWorld:mousedown (ev)
    self.label.text = "HELLO WORLD..."
    self.label_height = 44
    self.label_width  = 130
    self:resized()
end

function HelloWorld:mouseup (ev)
    self.label.text = self.original_text
    self.label_height = 32
    self.label_width  = 120
    self:resized()
end

local win = new (DocumentWindow)
function win:onclosebutton()
    if not win then return end
    win.visible = false
    win = nil
end

win.content = new (HelloWorld)
win.content.button.onclick = win.onclosebutton
win.visible = true

return win
