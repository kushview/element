/// Global objects.
// A collection of global objects 
// @classmod el.Globals
// @pragma nostrip

#include "lua-kv.hpp"
#include "Globals.h"

static Element::Globals* el_Globals_instance (lua_State* L)
{
    sol::state_view lua (L);
    auto& _G = lua.globals();
    auto g = _G.get_or<sol::userdata> ("el.globals", sol::lua_nil);
    return g.valid() ? (Element::Globals*) g.pointer() : nullptr;
}

static int el_Globals_userdata (lua_State* L)
{
    sol::state_view lua (L);
    auto& _G = lua.globals();
    sol::stack::push (L, _G.get_or<sol::userdata> ("el.globals", sol::lua_nil));
    return 1;
}

LUAMOD_API int luaopen_el_Globals (lua_State* L)
{
    using Element::Globals;
    sol::state_view lua (L);
    auto M = lua.create_table();
    M.new_usertype<Globals> ("Globals", sol::no_constructor,
        /// Returns el.Globals single instance
        // @function Globals.instance
        // @treturn el.Globals
        "instance",                 el_Globals_userdata,

        /// Get the current audio engine
        // @function Globals:audio_engine
        // @treturn el.AudioEngine
        "audio_engine",&Globals::getAudioEngine,

        /// Returns the command manager
        // @function Globals:command_manager
        // @treturn el.CommandManager
        "command_manager", &Globals::getCommandManager,

        /// Returns the device manager
        // @function Globals:device_manager
        // @treturn el.DeviceManager
        "device_manager", &Globals::getDeviceManager,

        /// Returns the mapping engine
        // @function Globals:mapping_engine
        // @treturn el.MappingEngine
        "mapping_engine", &Globals::getMappingEngine,

        /// Returns the midi engine
        // @function Globals:midi_engine
        // @treturn el.MidiEngine
        "midi_engine", &Globals::getMidiEngine,

        /// Returns the plugin manager
        // @function Globals:plugin_manager
        // @treturn el.PluginManager
        "plugin_manager", &Globals::getPluginManager,

        /// Returns the preset manager
        // @function Globals:preset_manager
        // @treturn el.PresetManager
        "preset_manager", &Globals::getPresetCollection,

        /// Returns the preset manager
        // @function Globals.session
        // @treturn el.Session
        "session", &Globals::getSession,

        /// Returns the preset manager
        // @function Globals:settings
        // @treturn el.Settings
        "settings", &Globals::getSettings
    );

    lua.script (R"(
        require ('el.CommandManager')
        require ('el.Node')
        require ('el.Session')
    )");

    sol::stack::push (L, kv::lua::remove_and_clear (M, "Globals"));
    return 1;
}
