--- Element Options
-- @module element.options

local M = {}
local options = {}

setmetatable (M, {
    __index = function (t, k)
        local v = options [k]
        if type(v) == 'function' then return v() end
        return options [k]
    end,

    __newindex = function (t, k, v)
        if type(options[k]) == 'function' then
            options [k] (v)
        else
            options [k] = v
        end
    end
})

return M
