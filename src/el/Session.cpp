/// Session object
// @classmod el.Session
// @pragma nostrip

#include "sol_helpers.hpp"
#include "session/node.hpp"
#include "session/session.hpp"

EL_PLUGIN_EXPORT int luaopen_el_Session (lua_State* L)
{
    using namespace element;

    sol::state_view lua (L);
    auto M = lua.create_table();
    M.new_usertype<Session> (
        "Session", sol::no_constructor, sol::meta_function::to_string, [] (Session* self) {
            String str = "Session";
            if (self->getName().isNotEmpty())
                str << ": " << self->getName();
            return str.toStdString(); },

        sol::meta_function::length,
        [] (Session* self) { return self->getNumGraphs(); },
        sol::meta_function::index,
        [] (Session* self, int index) {
            return isPositiveAndBelow (--index, self->getNumGraphs())
                       ? std::make_shared<Node> (self->getGraph (index).getValueTree(), false)
                       : std::shared_ptr<Node>();
        },

        /// Attributes.
        // @section attributes

        /// Session name.
        // @field Session.name
        "name",
        sol::property ([] (Session& self, const char* name) -> void { self.setName (String::fromUTF8 (name)); }, [] (Session& self) -> std::string { return self.getName().toStdString(); }),

        /// Methods.
        // @section methods

        /// Convert to an XML string.
        // @function Session:toxmlstring
        // @return XML formatted string of the session
        "toxmlstring",
        [] (Session& self) -> std::string {
            auto tree = self.getValueTree().createCopy();
            Node::sanitizeRuntimeProperties (tree, true);
            return tree.toXmlString().toStdString();
        },

        /// Save the state.
        // Will save state for all loaded plugins as well
        // @function Session:savestate
        "savestate",
        &Session::saveGraphState,

        /// Restore state.
        // Restores all plugin states
        // @function Session:restorestate
        "restorestate",
        &Session::restoreGraphState

#if 0
        "clear",                    &Session::clear,
        "get_num_graphs",           &Session::getNumGraphs,
        "get_graph",                &Session::getGraph,
        "get_active_graph",         &Session::getActiveGraph,
        "get_active_graph_index",   &Session::getActiveGraphIndex,
        "add_graph",                &Session::addGraph,
#endif
    );

    sol::stack::push (L, element::lua::remove_and_clear (M, "Session"));
    return 1;
}
