
#include <element/element.h>
#include <element/graph.hpp>
#include "./nodetype.hpp"

// clang-format off

EL_PLUGIN_EXPORT int luaopen_el_Node (lua_State* L)
{
    using namespace element;
    using namespace sol;
    auto M = element::lua::new_nodetype<element::Graph> (L, "Graph");
    sol::stack::push (L, element::lua::remove_and_clear (M, "Graph"));
    return 1;
}
