--- Test Script
-- @name Test Script
-- @author Michael Fisher
-- @description A script for testing purposes

local function MyObject()
    local self = {}
    function self.print()
        print (self)
    end
    return self
end

local obj = MyObject()
obj:print()
obj.print()
obj = MyObject()
obj:print()
obj.print()
