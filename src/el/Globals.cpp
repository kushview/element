/// Global objects.
// A collection of global objects 
// @classmod el.Globals
// @pragma nostrip

#include "lua.hpp"
#include "sol_helpers.hpp"

#include "session/commandmanager.hpp"
#include "session/pluginmanager.hpp"
#include "session/presetmanager.hpp"
#include "globals.hpp"
#include "settings.hpp"

static int el_Globals_userdata (lua_State* L)
{
    sol::state_view lua (L);
    auto& _G = lua.globals();
    sol::stack::push (L, _G.get_or<sol::userdata> ("el.globals", sol::lua_nil));
    return 1;
}

EL_PLUGIN_EXPORT int luaopen_el_Globals (lua_State* L)
{
    using Element::Globals;
    sol::state_view lua (L);
    auto M = lua.create_table();
    M.new_usertype<Globals> ("Globals", sol::no_constructor,
        /// Class Methods.
        // @section classmethods

        /// Returns el.Globals single instance
        // @function Globals.instance
        // @treturn el.Globals
        "instance",                 el_Globals_userdata,

        /// Methods.
        // @section methods

        /// Get the current audio engine
        // @function Globals:audioengine
        // @treturn el.AudioEngine
        "audioengine",&Globals::getAudioEngine,

        /// Returns the command manager
        // @function Globals:commandmanager
        // @treturn el.CommandManager
        "commandmanager", &Globals::getCommandManager,

        /// Returns the device manager
        // @function Globals:devicemanager
        // @treturn el.DeviceManager
        "devicemanager", &Globals::getDeviceManager,

        /// Returns the mapping engine
        // @function Globals:mappingengine
        // @treturn el.MappingEngine
        "mappingengine", &Globals::getMappingEngine,

        /// Returns the midi engine
        // @function Globals:midiengine
        // @treturn el.MidiEngine
        "midiengine", &Globals::getMidiEngine,

        /// Returns the plugin manager
        // @function Globals:pluginmanager
        // @treturn el.PluginManager
        "pluginmanager", &Globals::getPluginManager,

        /// Returns the preset manager
        // @function Globals:presetmanager
        // @treturn el.PresetManager
        "presetmanager", &Globals::getPresetManager,

        /// Returns the preset manager
        // @function Globals:session
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

    sol::stack::push (L, element::lua::remove_and_clear (M, "Globals"));
    return 1;
}
