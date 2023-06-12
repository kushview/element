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
#include <element/pluginmanager.hpp>
#include "session/presetmanager.hpp"
#include <element/context.hpp>

using namespace juce;
namespace lua = element::lua;
// clang-format off

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
                            "instance", el_Context_userdata,
                            "commands", &Context::getCommandManager,
                            "audio",    &Context::getAudioEngine,
                            "devices",  &Context::getDeviceManager,
                            "mapping",  &Context::getMappingEngine,
                            "midi",     &Context::getMidiEngine,
                            "plugins",  &Context::getPluginManager,
                            "presets",  &Context::getPresetManager,
                            "session",  &Context::getSession,
                            "settings", &Context::getSettings);

    lua.script (R"(
        require ('el.CommandManager')
        require ('el.Node')
        require ('el.Session')
    )");

    sol::stack::push (L, lua::remove_and_clear (M, "Context"));
    return 1;
}
// clang-format on
