project     = 'Element'
title       = 'Element Lua Documentation'
description = 'Element Lua Documentation'
file = {
    'libs/element/lua',
    'scripts',
    'src/engine/MidiPipe.cpp',
    exclude = {
        'docs/examples',
        'libs/element/lua/element.lua',
        'libs/element/lua/el/audio_buffer.hpp',
        'libs/element/lua/el/midi_buffer.hpp',
        'libs/element/lua/el/sol_helpers.hpp',
        'libs/element/lua/el/lua-kv.hpp',
        'libs/element/lua/el/lua-kv.h',
    },
}
examples = {
    'docs/examples/helloworld.lua',
    'docs/examples/sysex.lua',
    'docs/examples/sysexui.lua',
    'scripts/amp.lua',
    'scripts/ampui.lua',
    'scripts/channelize.lua'
}
dir         = 'build/doc/lua'
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
