--- Show a custom widget in a window
-- @script window
-- @usage
-- local win = element.script ('window')

local widget = require ('element.widget')
local window = require ('element.window')
local HelloWorld = widget.derive()

function HelloWorld:init()
    self.name = "Hello World"
    self:setsize (640, 360)
end

function HelloWorld:paint (g)
    g:fillAll (0xff545454)
    g:setColour (0xffffffff)
    g:drawText (self.name, 0, 0, self.width, self.height)
end

local win = window.new (HelloWorld.new())

win.onclosebutton = function (self)
    self.visible  = false
    self.desktop  = false
end

win.visible = true
win.desktop = true

return  win
