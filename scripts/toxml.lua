--- Test Script
-- Convert something to XML
-- @script toxml
-- @usage
-- local xml = element.script ('toxml')
-- print (xml)

local widget = require ('element.widget')
local Square = widget.derive()

function Square:init()
    print("Square:init()")
end

function Square:paint (g)
    g:fillall (0xff0000ff)
end

local Square2 = widget.derive (Square)
function Square2:init()
    print("Square2:init()")
end

local Square3 = widget.derive (Square2)
function Square3:init()
    print("Square3:init()")
end

local obj = Square3.new()
obj:setsize (640, 360)
obj:setvisible (true)
obj:addtodesktop()
return obj
