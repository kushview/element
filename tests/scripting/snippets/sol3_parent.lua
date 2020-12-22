
local object = require ('kv.object')

local function dumpt (obj) 
    for k,v in pairs (obj) do print (k,v) end
end

local function dumpmt (obj)
    dumpt (getmetatable (obj))
end

local function runtest()
    begintest ("object.new (Parent)")
    local parent = object.new (Parent)
    expect (parent.name == "Parent Object")
    parent.name = "Parent"
    expect (parent.name == "Parent")
   
    local Derived = object (Parent, {
        name = {
            get = function (self)
                return rawget (self, '_name') or "Derived" 
            end
        }
    })

    function Derived:init()
        Parent.init (self)
    end

    function Derived:get()
        local impl = getmetatable(self).__impl
        return impl:get() * 2.0
    end

    local d = object.new (Derived)
    local impl = getmetatable(d).__impl

    begintest ("Derived: rawset")
    d.name2 = "Alternate Name"
    expect (d.name2 == "Alternate Name")
    expect (d.name == "Derived", tostring (d.name))

    begintest ("readonly att")
    local r,e = pcall (function() 
        d.name = d.name2
    end)
    expect (r == false)
    expect (d.name ~= d.name2, d.name)

    begintest ("Derived:get")

    expect (type(d.get) == 'function', "wrong type: " .. type(d.get))
    expect (impl:get() == 100, impl:get())
    expect (d:get() == 200, d:get())

    begintest ("changed")
    expect (impl.changed == nil)
    d.changed = function()
        print ("value changed: " .. tostring (impl:get()))
    end

    -- expect (impl.changed ~= nil)
    begintest ("Derived:set")
    d:set (50)
    expect (d:get() == 100, d:get())
end

runtest()
collectgarbage()
