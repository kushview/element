/// Session object.
// @classmod el.Commands
// @pragma nostrip

#include "sol_helpers.hpp"
#include "session/commandmanager.hpp"
#include "commands.hpp"

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
        [] (CommandID cmd) { return Commands::toString (cmd).toStdString(); },

        sol::base_classes,
        sol::bases<juce::ApplicationCommandManager>());

    sol::stack::push (L, element::lua::remove_and_clear (M, "Commands"));
    return 1;
}
