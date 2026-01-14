// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

/// The Graph Model.
// Representation of a Graph. Is a @{el.Node}
// @classmod el.Graph
// @pragma nostrip

#include <element/element.h>
#include <element/graph.hpp>
#include "./nodetype.hpp"

// clang-format off
EL_PLUGIN_EXPORT int luaopen_el_Graph (lua_State* L)
{
    using namespace element;
    using namespace sol;
    
    auto M = element::lua::new_nodetype<element::Graph> (L, "Graph",
        sol::meta_function::to_string, [](Graph& self) { return lua::to_string (self, "Graph"); },
        
        /// Methods.
        // @section methods
        
        /// Returns true if this graph has a view script.
        // @function Graph:hasViewScript
        // @return bool True if yes
        "hasViewScript",  &Graph::hasViewScript,

        /// Returns the view script if available.
        // @function Graph:viewScript
        // @treturn el.Script True if yes
        "viewScript",     &Graph::findViewScript
    );

    sol::stack::push (L, element::lua::removeAndClear (M, "Graph"));
    return 1;
}
