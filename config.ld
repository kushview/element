project     = 'Element'
title       = 'Element Lua Documentation'
description = 'Element Lua Documentation'
file = { 
    'libs/lua-kv/src',
    'libs/element/src',
    'scripts',
    'src/scripting/LuaBindings.cpp',
    'src/scripting/JuceBindings.cpp',
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

---[[
custom_tags = {
    {
        "property",
        title = "Properties",
        hidden = false,
        format = function (data)
            return data .. ""
        end
    }
}

custom_display_name_handler = function (item, default_handler)
    if item.tags.property then
        -- for k,v in pairs(item) do
        --     print(k, v)
        -- end
        item.args = nil
        return item.name
    end
    return default_handler(item)
end
--]]