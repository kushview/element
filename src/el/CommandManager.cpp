/// Session object.
// @classmod el.Commands
// @pragma nostrip

// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/element.h>
#include <element/ui/commands.hpp>
#include "sol_helpers.hpp"

EL_PLUGIN_EXPORT int luaopen_el_Commands (lua_State* L)
{
    using namespace element;
    sol::state_view lua (L);
    auto M = lua.create_table();
    M.new_usertype<Commands> (
        "Commands", sol::no_constructor, "invokeDirectly", &Commands::invokeDirectly,

        "standard",
        []() {
            std::vector<int> ids;
            for (int i : Commands::getAllCommands())
                ids.push_back (i);
            return ids;
        },

        /// Convert standard ID to string slug.
        // @function Commands.tostring
        // @see Commands.standard
        "toString",
        [] (juce::CommandID cmd) { return Commands::toString (cmd).toStdString(); },

        sol::base_classes,
        sol::bases<juce::ApplicationCommandManager>());

    sol::stack::push (L, element::lua::removeAndClear (M, "Commands"));
    return 1;
}
