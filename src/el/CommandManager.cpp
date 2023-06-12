/// Session object.
// @classmod el.CommandManager
// @pragma nostrip

#include "sol_helpers.hpp"
#include "session/commandmanager.hpp"
#include "commands.hpp"

EL_PLUGIN_EXPORT int luaopen_el_CommandManager (lua_State* L)
{
    using namespace element;
    sol::state_view lua (L);
    auto M = lua.create_table();
    M.new_usertype<CommandManager> (
        "CommandManager", sol::no_constructor, "invokeDirectly", &CommandManager::invokeDirectly,

        "standard",
        []() {
            std::vector<int> ids;
            for (int i : Commands::getAllCommands())
                ids.push_back (i);
            return ids;
        },

        /// Convert standard ID to string slug.
        // @function CommandManager.tostring
        // @see CommandManager.standard
        "toString",
        [] (CommandID cmd) { return Commands::toString (cmd).toStdString(); },

        sol::base_classes,
        sol::bases<ApplicationCommandManager>());

    sol::stack::push (L, element::lua::remove_and_clear (M, "CommandManager"));
    return 1;
}
