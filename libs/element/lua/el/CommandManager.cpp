/// Session object.
// @classmod el.CommandManager
// @pragma nostrip

#include "lua-kv.hpp"
#include "session/CommandManager.h"
#include "Commands.h"

LUAMOD_API int luaopen_el_CommandManager (lua_State* L)
{
    using namespace Element;
    sol::state_view lua (L);
    auto M = lua.create_table();
    M.new_usertype<CommandManager> ("CommandManager", sol::no_constructor,
        /// Invoke a command directly
        // @int cmd Command ID to invoke
        // @bool async Invoke now or aysnc
        // @function CommandManager:invoke_directly
        // @treturn bool True if success
        "invoke_directly",   &CommandManager::invokeDirectly,

        // Clears the current list of all commands.
        // Note that this will also clear the contents of the KeyPressMappingSet.
        // @function CommandManager:clear
        // "clear", &CommandManager::clearCommands,

        // Adds a command to the list of registered commands.
        // @function CommandManager:register
        // "register", &CommandManager::registerCommand,

        // Returns number of registered commands
        // @function CommandManager:size
        // "size", &CommandManager::getNumCommands,

        /// Tell the manager a command has changed.
        // Call this if you modify the details of a command.
        "command_changed", &CommandManager::commandStatusChanged,

        /// Retursn list of standard command IDs
        // @function CommandManager.standard
        "standard", []() {
            std::vector<int> ids;
            for (int i : Commands::getAllCommands())
                ids.push_back (i);
            return ids;
        },

        /// Convert standard ID to string slug.
        // @function CommandManager.tostring
        // @see CommandManager.standard
        "tostring", [](CommandID cmd) {
            return Commands::toString(cmd).toStdString();
        },

        sol::base_classes, sol::bases<ApplicationCommandManager>()
    );

    sol::stack::push (L, kv::lua::remove_and_clear (M, "CommandManager"));
    return 1;
}
