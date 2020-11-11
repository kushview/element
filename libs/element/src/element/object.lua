local M = {}

--- Make proxy object with property support.
-- Notes:
--   If key is found in <getters> (or <setters>), then
--     corresponding function is used, else lookup turns to the
--     <class> metatable (or first to <priv> if <is_expose_private> is true).
--   Given a proxy object <self>, <priv> can be obtained with
--     getmetatable(self).priv .
-- @param class - metatable acting as the object class.
-- @param priv - table containing private data for object.
-- @param getters - table of getter functions
--                  with keys as property names. (default is nil)
-- @param setters - table of setter functions,
--                  with keys as property names. (default is nil)
-- @param expose - Boolean whether to expose <priv> through proxy.
--                  (default is nil/false)
-- @version 3 - 20060921 (D.Manura)
function M.instance (class, priv, getters, setters, expose)
    setmetatable (priv, class)  -- fallback priv lookups to class
    local fallback = expose and priv or class

    local index = getters and
        function (self, key)
            -- read from getter, else from fallback
            local func = getters[key]
            if func then return func(self) else return fallback[key] end
        end
        or fallback  -- default to fast property reads through table

    local newindex = setters and
        function (self, key, value)
            -- write to setter, else to proxy
            local func = setters[key]
            if func then func(self, value)
            else rawset (self, key, value) end
        end
        or fallback  -- default to fast property writes through table

    -- create metatable for proxy object
    local proxy_mt = {         
        __newindex = newindex,
        __index = index,
        priv = priv
    }

    local self = setmetatable({}, proxy_mt)  -- create proxy object
    return self
end

return M
