project     = 'Element'
title       = 'Element Lua Documentation'
description = 'Element Lua Documentation'
file = { 
    'libs/lua-kv/src',
    'libs/element/element',
    'scripts',
    'src/scripting/LuaBindings.cpp',
    exclude = {
        'libs/lua-kv/src/lua-kv.h'
    },
}
examples = {
    'scripts'
}
dir         = 'build/doc/lua'
output      = 'index'
merge       = true
all         = false

--[[
new_type ('element', "Element", true)

custom_tags = {
    {
        "custo",
        hidden = false
    }
}

custom_display_name_handler = function (item, default_handler)
    if item.tags.element then
        return item.name .. ' [element]'
    elseif item.tags.custo then
        return item.name .. ' : custo'
    end
    return default_handler(item)
end
]]--