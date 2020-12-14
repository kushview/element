--- Show a custom widget in a window.
-- The return value is the displayed window or nil
-- @script window
-- @usage
-- local win = element.script ('window')

local object = require ('kv.object')
local Widget = require ('kv.Widget')
local Window = require ('kv.Window')

local colors = {
    text = 0xffffffff,
    background = 0xff545454
}

local Label = object (Widget, {
    text = {
        set = function (self, value)
            getmetatable(self).text = value
            self:repaint()
        end,
        get = function (self)
            return getmetatable(self).text or ""
        end
    }
})

function Label:init()
    Widget.init (self)
    self.text = ""
    self:resize (100, 100)
end

function Label:paint (g)
    g:set_color (colors.text)
    g:draw_text (self.text, 0, 0, self.width, self.height)
end

local HelloWorld = object (Widget)

function HelloWorld:init()
    Widget.init (self)
    self.name = "HelloWorld"
    self.label = self:add (object.new (Label))
    self.label.text = "Hello world..."
    self.original_text = self.label.text
    self.label_height = 32
    self.label_width  = 120
    self:resize (640, 360)
end

function HelloWorld:resized()
    self.label.bounds = {
        x = (self.width / 2) - (self.label_width / 2),
        y = (self.height / 2) - (self.label_height / 2),
        width = self.label_width,
        height = self.label_height
    }
end

function HelloWorld:paint (g)
    g:fill_all (colors.background)
end

function HelloWorld:mouse_down (ev)
    self.label.text = "HELLO WORLD..."
    self.label_height = 44
    self.label_width  = 130
    self:resized()
end

function HelloWorld:mouse_up (ev)
    self.label.text = self.original_text
    self.label_height = 32
    self.label_width  = 120
    self:resized()
end

local hw  = object.new (HelloWorld)
local win = object.new (Window)
win.name = "Hello World - Element Lua"
win:set_widget (hw)

function win:onclosebutton()
    self.visible  = false
    self.desktop  = false
    self:set_widget (nil)
    win.impl = nil
    win = nil
    hw.impl = nil
    hw = nil
end

win.desktop = true
win.visible = true
