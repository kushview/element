// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

/// The main context in which Element is running.
// The main Element context.  A collection of "global" objects.
// @classmod el.Context
// @pragma nostrip

#include <element/ui/commands.hpp>
#include <element/context.hpp>
#include <element/devices.hpp>
#include <element/plugins.hpp>
#include <element/settings.hpp>

#include "lua.hpp"
#include "sol_helpers.hpp"
#include "engine/midiengine.hpp"
#include "engine/mappingengine.hpp"
#include "presetmanager.hpp"

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
        /// Returns the single instance.
        // @function Context.instance
        // @treturn el.Context
        // @within Class Methods
        "instance", el_Context_userdata,

        /// Returns the active el.Session.
        // @function Context:session
        // @treturn el.Session
        // @within Instance Methods
        "session",  &Context::session,

        "audio",    &Context::audio,
        "devices",  &Context::devices,
        "mapping",  &Context::mapping,
        "midi",     &Context::midi,
        "plugins",  &Context::plugins,
        "presets",  &Context::presets,
        "settings", &Context::settings);

    lua.script (R"(
        require ('el.Node')
        require ('el.Session')
    )");

    sol::stack::push (L, lua::removeAndClear (M, "Context"));
    return 1;
}
// clang-format on
