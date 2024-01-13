project     = 'Element'
title       = 'Element Lua API'
description = "API reference to Element's scripting engine"
file = {
    '../src/el',
    '../src/engine/midipipe.cpp',
    exclude = {
        '../src/el/midi_buffer.hpp',
        '../src/el/sol_helpers.hpp',
        '../src/el/vector.c',
        '../src/el/color.lua',
        '../src/el/session.lua',
        '../src/el/Parameter.cpp'
    }
}

examples = {
--    '../docs/examples/helloworld.lua',
--    '../docs/examples/sysex.lua',
--    '../docs/examples/sysexui.lua',
    '../scripts/amp.lua',
    '../scripts/ampui.lua',
    '../scripts/channelize.lua'
}
output      = 'index'
merge       = true
all         = false

add_language_extension ('.hpp', 'c')
add_language_extension ('.ipp', 'c')

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
    if item.tags.kind then
        return item.name
    end
    return default_handler (item)
end
--]]
