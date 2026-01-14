// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

/// UI Commands object.
// @classmod el.Commands
// @pragma nostrip

#include <element/element.h>
#include <element/ui/commands.hpp>
#include "sol_helpers.hpp"

// clang-format off
EL_PLUGIN_EXPORT 
int luaopen_el_Commands (lua_State* L)
{
    using namespace element;
    sol::state_view lua (L);
    auto M = lua.create_table();
    M.new_usertype<Commands> (
        "Commands", sol::no_constructor, 
        
        /// Invoke a command directly.
        // @function Commands:invokeDirectly
        // @int command The CommandID to invoke.
        // @bool async Invoke async or not.
        "invokeDirectly", &Commands::invokeDirectly,

        /// Returns a list of all standard command IDs.
        // @function Commands.standard
        // @see Commands.standard
        "standard", []() {
            std::vector<lua_Integer> ids;
            for (int i : Commands::getAllCommands())
                ids.push_back (i);
            return ids;
        },

        /// Convert standard ID to String.
        // @function Commands.toString
        // @int command The command ID to convert.
        // @see Commands.standard
        "toString", [] (juce::CommandID cmd) { 
            return Commands::toString (cmd).toStdString();
        },

        sol::base_classes,
        sol::bases<juce::ApplicationCommandManager>());

    sol::stack::push (L, element::lua::removeAndClear (M, "Commands"));
    return 1;
}
