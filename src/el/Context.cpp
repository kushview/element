/// The main context in which Element is running.
// A collection of global objects.
// @classmod el.Context
// @pragma nostrip

#include "lua.hpp"
#include "sol_helpers.hpp"

#include <element/settings.hpp>

#include "engine/midiengine.hpp"
#include "engine/mappingengine.hpp"
#include "session/commandmanager.hpp"
#include "session/pluginmanager.hpp"
#include "session/presetmanager.hpp"
#include <element/context.hpp>

using namespace juce;
namespace lua = element::lua;

static int el_Context_userdata (lua_State* L)
{
    sol::state_view lua (L);
    auto& _G = lua.globals();
    sol::stack::push (L, _G.get_or<sol::userdata> ("el.context", sol::lua_nil));
    return 1;
}

EL_PLUGIN_EXPORT int luaopen_el_Context (lua_State* L)
{
    using element::Context;
    sol::state_view lua (L);
    auto M = lua.create_table();
    M.new_usertype<Context> ("Context", sol::no_constructor,
                             /// Class Methods.
                             // @section classmethods

                             /// Returns el.Context single instance
                             // @function Globals.instance
                             // @treturn el.Context
                             "instance",
                             el_Context_userdata,

                             /// Methods.
                             // @section methods

                             /// Get the current audio engine
                             // @function Globals:audioengine
                             // @treturn el.AudioEngine
                             "audioengine",
                             &Context::getAudioEngine,

                             /// Returns the command manager
                             // @function Globals:commandmanager
                             // @treturn el.CommandManager
                             "commandmanager",
                             &Context::getCommandManager,

                             /// Returns the device manager
                             // @function Globals:devicemanager
                             // @treturn el.DeviceManager
                             "devicemanager",
                             &Context::getDeviceManager,

                             /// Returns the mapping engine
                             // @function Globals:mappingengine
                             // @treturn el.MappingEngine
                             "mappingengine",
                             &Context::getMappingEngine,

                             /// Returns the midi engine
                             // @function Globals:midiengine
                             // @treturn el.MidiEngine
                             "midiengine",
                             &Context::getMidiEngine,

                             /// Returns the plugin manager
                             // @function Globals:pluginmanager
                             // @treturn el.PluginManager
                             "pluginmanager",
                             &Context::getPluginManager,

                             /// Returns the preset manager
                             // @function Globals:presetmanager
                             // @treturn el.PresetManager
                             "presetmanager",
                             &Context::getPresetManager,

                             /// Returns the preset manager
                             // @function Globals:session
                             // @treturn el.Session
                             "session",
                             &Context::getSession,

                             /// Returns the preset manager
                             // @function Globals:settings
                             // @treturn el.Settings
                             "settings",
                             &Context::getSettings);

    lua.script (R"(
        require ('el.CommandManager')
        require ('el.Node')
        require ('el.Session')
    )");

    sol::stack::push (L, lua::remove_and_clear (M, "Context"));
    return 1;
}
