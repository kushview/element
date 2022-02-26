--- Define objects with attributes.
-- @module kv.object
-- @usage
-- local Animal = object()
local M = {}

local function lookup (T, name)
    while T do
        if rawget(T, name) then return rawget(T, name), T end
        T = getmetatable(T).__base
    end
    return false, false
end

local function lookupmeta (T, name)
    while T do
        local mt = getmetatable (T)
        if mt then
            local val = rawget (mt, name)
            if val then return val, T end
        end
        T = mt and mt.__base or nil
    end
    return nil, nil
end

--- Create a proxy for userdata.
-- @tparam table T Derived type
-- @tparam table U Type table which created userdata
-- @tparam userdata _impl The userdata to wrap.
local function make_userdata_proxy (T, U, _impl)
    local impl = _impl
    assert (type(impl) == 'userdata', "not userdata")
    local atts = getmetatable(T).__atts or false

    local props = {}
    for _,k in ipairs(getmetatable(U).__props or {}) do
        props[k] = true
    end
    
    local methods = getmetatable(U).__methods or {}
    local proxy,proxy_mt = {}, {}
    local exported = {}

    for _, m in ipairs (methods) do
        exported[m] = function (self, ...)
            return impl[m] (impl, ...)
        end
    end

    proxy_mt.__impl = impl

    function proxy_mt.__gc (self)
        atts = nil
        exported = nil
        methods = nil
        impl = nil
        proxy_mt.__impl = nil
        proxy_mt = nil
        proxy = nil
    end

    function proxy_mt.__index (self, k)
        local val = rawget (self, k)
        if val then return val end

        if atts then
            val = atts[k];
            if val and val.get then
                return val.get (self) 
            end
        end

        if props[k] then return impl[k] end

        val = T ~= U and T[k] or nil;        
        if val then return val end
        
        val = exported[k]; 
        if val then return val end

        return nil
    end

    function proxy_mt.__newindex (self, k, v)
        local a = atts and atts [k] or false
        if a then
            if a.set then
                a.set (self, v)
            else
                error ("cannot modify readonly attribute")
            end
            return
        end

        if props[k] then impl[k] = v; return end

        rawset (self, k, v)
    end

    return setmetatable (proxy, proxy_mt)
end

local function make_table_proxy (T, impl, expose)
    expose = expose or false
    impl = impl or {}

    local atts = getmetatable(T).__atts or false
    local fallback = expose and impl or T

    return setmetatable ({}, {
        __impl  = impl or {},
        __index = atts and function (obj, k)
            local p = atts [k]
            if p and p.get then
                return p.get (obj)
            else
                return fallback[k]
            end
        end
        or fallback,

        __newindex = atts and function (obj, k, v)
            local p = atts [k]
            if p and p.set then
                p.set (obj, v)
            else
                rawset (obj, k, v)
            end
        end
        or fallback
    })
end

local function instantiate (T, ...)
    local newuserdata, U = lookupmeta (T, '__newuserdata')
    
    local proxy = {}
    if type(newuserdata) == 'function' and
       type(U) == 'table'
    then
        local obj = newuserdata()
        proxy = make_userdata_proxy (T, U, obj)
    else
        proxy = make_table_proxy (T, {})
    end

    if 'function' == type (T['init']) then
        T.init (proxy, ...)
    end

    return proxy
end

local function define_type (...)
    local nargs = select ("#", ...)
    local B = {}
    local D = {}
    local atts = {}

    if nargs == 0 then
        atts = {}

    elseif nargs == 1 and 'table' == type (select (1, ...)) then
        local param = select (1, ...)
        if getmetatable(param) then
            B = param
            if getmetatable(B).__atts then
                for k,v in pairs (getmetatable(B).__atts) do
                    atts[k] = v
                end
            end
        else
            atts = param
        end

    elseif nargs > 1 then
        B = select (1, ...)
        if getmetatable(B).__atts then
            for k,v in pairs (getmetatable(B).__atts) do
                atts[k] = v
            end
        end
        for k,v in pairs (select (2, ...)) do
            atts[k] = v
        end
    end
    
    local result, err = pcall (function()
        for k, v in pairs (B) do
            D[k] = v 
        end
    end)
    
    return setmetatable (D, {
        __atts  = atts,
        __base  = B
    })
end

--- Define a new object type.
-- Call when you need to define a custom object. You can also invoke the module
-- directly, which is an alias to `define`.
-- @class function
-- @name object.define
-- @param base
-- @treturn table The new object type table
-- @usage
-- local object = require ('kv.object')
-- -- Verbose method
-- local Animal = object.define()
-- -- Calling the module
-- local Cat = object (Animal)
M.define = define_type

--- Create a new instance of type `T`.
-- @tparam table T The object type to create
-- @tparam any ... Arguments passed to `T:init`
-- @treturn table The newly created object
function M.new (T, ...)
    return instantiate (T, ...)
end

--- Reference some userdata.
-- Only call this on userdata allocated outside of lua.
-- @tparam table T The type of userdata
-- @tparam userdata obj Instance allocated in C/C++
-- @bool init Set false to skip calling T.init (default true)
-- @return table Proxy object for `obj` 
function M.ref (T, obj, init)
    init = init ~= nil and init or true
    if type (T)   ~= 'table'    then error ("param #1 is not a table") end
    if type (obj) ~= 'userdata' then error ("param #2 is not userdata") end
    return make_userdata_proxy (T, T, obj)
end

setmetatable (M, {
    __call = function (O, ...)
        return define_type (...)
    end
})

return M
