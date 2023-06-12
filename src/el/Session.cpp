#include <element/element.h>
#include <element/node.hpp>
#include <element/session.hpp>

#include "sol_helpers.hpp"

// clang-format off
EL_PLUGIN_EXPORT int luaopen_el_Session (lua_State* L)
{
    using namespace element;

    sol::state_view lua (L);
    auto M = lua.create_table();
    M.new_usertype<Session> ("Session", sol::no_constructor, 
        sol::meta_function::to_string, [](Session& self) { return lua::to_string (self, "Session"); },
        sol::meta_function::length, [] (Session* self) { return self->getNumGraphs(); },
        sol::meta_function::index, [] (Session* self, int index) {
            return isPositiveAndBelow (--index, self->getNumGraphs())
                       ? std::make_shared<Node> (self->getGraph (index).getValueTree(), false)
                       : std::shared_ptr<Node>();
        },

        "name", [](Session& self) { return self.getName().toStdString(); },

        "toXmlString", [] (Session& self) -> std::string {
            auto tree = self.getValueTree().createCopy();
            Node::sanitizeRuntimeProperties (tree, true);
            return tree.toXmlString().toStdString();
        },

        "saveState",    &Session::saveGraphState,
        "restoreState", &Session::restoreGraphState

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
