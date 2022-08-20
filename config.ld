project     = 'Element'
title       = 'Element Lua Documentation'
description = 'Element Lua Documentation'
file = {
    'src/el',
    'scripts',
    'src/engine/midipipe.cpp',
    exclude = {
        'docs/examples',
        'src/el/audio_buffer.hpp',
        'src/el/midi_buffer.hpp',
        'src/el/sol_helpers.hpp',
        'src/el/lua-kv.hpp',
        'src/el/lua-kv.h'
    }
}
examples = {
    'docs/examples/helloworld.lua',
    'docs/examples/sysex.lua',
    'docs/examples/sysexui.lua',
    'scripts/amp.lua',
    'scripts/ampui.lua',
    'scripts/channelize.lua'
}
dir         = 'build-native/docs/lua'
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
