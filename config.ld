project     = 'Element'
title       = 'Element Lua Documentation'
description = 'Element Lua Documentation'
file = {
    'libs/lua-kv/src/kv',
    'libs/lua-kv/src/juce_rectangle.hpp',
    'libs/lua-kv/kv/lua/widget.hpp',
    'libs/element/lua',
    'scripts',
    exclude = {
        'libs/element/lua/element.lua',
        'libs/lua-kv/src/lua-kv.h',
        'libs/lua-kv/src/kv/vector.c',
        'libs/lua-kv/src/kv/File.cpp'
    },
}
examples = {
    'scripts/helloworld.lua',
    'scripts/amp.lua'
}
dir         = 'build/doc/lua'
output      = 'index'
merge       = true
all         = false

add_language_extension ('.hpp', 'c')

---[[
custom_tags = {
    {
        "property",
        title = "Properties",
        hidden = false,
        format = function (data)
            return data .. ""
        end
    } , {
        "kind",
        title = "Script Type",
        hidden = false,
    }
}

custom_display_name_handler = function (item, default_handler)
    if item.tags.kind then
        return item.name
    end
    return default_handler (item)
end
--]]
