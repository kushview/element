--- Test Script
-- @name    Test Script
-- @element Session.Signal
-- @author  Michael Fisher
-- @module  element.TestModule
-- @description A script for testing purposes

local MyPlugin = {
}

function MyPlugin.MyObject()
    local self = {}

    function self.__tostring()
        return 'MyObject tostring'
    end

    function self.print()
        print (self)
    end

    function self.test()
        local obj = self
        obj.print()
        obj = MyPlugin.MyObject()
        obj.print()
    end

    return self
end

return MyPlugin
