// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

/// The Node Model.
// Representation of a Node. Is an el.Model
// @classmod el.Node
// @pragma nostrip

#include <element/element.h>
#include <element/node.hpp>
#include "./nodetype.hpp"

// clang-format off
EL_PLUGIN_EXPORT int luaopen_el_Node (lua_State* L)
{
    using namespace element;
    using namespace sol;
    
    auto M = element::lua::new_nodetype<element::Node> (L, "Node",
        sol::meta_method::to_string, [](Node& self) { return lua::to_string (self, "Node"); },

        /// Returns true if this node has an editor.
        // @function Node:hasEditor
        // @within Methods
        // @return bool True if yes.
        "hasEditor", &Node::hasEditor
    );

    sol::stack::push (L, M);
    return 1;
}
