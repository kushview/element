/// Node object.
// @classmod el.Node
// @pragma nostrip

#include "lua-kv.hpp"
#include "session/Node.h"

EL_PLUGIN_EXPORT int luaopen_el_Node (lua_State* L) {
    using namespace Element;
    using namespace sol;

    sol::state_view lua (L);
    auto M = lua.create_table();
    M.new_usertype<Node> ("Node", no_constructor,
        meta_function::to_string, [](const Node& self) -> std::string {
            String str = self.isGraph() ? "Graph" : "Node";
            if (self.getName().isNotEmpty())
                str << ": " << self.getName();
            return str.toStdString();
        },
        meta_function::length,  &Node::getNumNodes,
        meta_function::index,   [](Node* self, int index)
        {
            const auto child = self->getNode (index - 1);
            return child.isValid() ? std::make_shared<Node> (child.getValueTree(), false)
                                   : std::shared_ptr<Node>();
        },

        /// Attributes.
        // @section Attributes

        /// Node name.
        // @field Node.name
        // @within Attributes
        "name", property (
            [](Node* self) { return self->getName().toStdString(); },
            [](Node* self, const char* name) { self->setProperty (Tags::name, String::fromUTF8 (name)); }
        ),
        
        "missing",              readonly_property (&Node::isMissing),
        "uuid",                 readonly_property (&Node::getUuid),
        "uuidstring",           readonly_property (&Node::getUuidString),
        "nodeid",               readonly_property (&Node::getNodeId),
        "type",                 readonly_property (&Node::getNodeType),

        /// Methods.
        // @section methods

        /// Returns true if a valid node.
        // @function Node:isvalid
        // @within Methods
        "isvalid",              &Node::isValid,

        /// True if enabled.
        // @function Node:isenabled
        // @within Methods
        "isenabled",            &Node::isEnabled,

        /// True if bypassed.
        // @function Node:isbypassed
        "isbypassed",           &Node::isBypassed,

        /// True if muted.
        // @function Node:ismuted
        "ismuted",              &Node::isMuted,

        /// Display name.
        // Will be a user-defined name or name provided by the plugin.
        // @function Node:displayname
        // @treturn string
        "displayname",          [](Node* self) { return self->getDisplayName().toStdString(); },

        /// Plugin name.
        // Name provided by the plugin.
        // @function Node:pluginname
        // @treturn string
        "pluginname",           [](Node* self) { return self->getPluginName().toStdString(); },

        /// True if this node is a graph.
        // @function Node:isgraph
        "isgraph",              &Node::isGraph,

        /// True if this node is a root graph.
        // @function Node:isroot
        "isroot",               &Node::isRootGraph,

        /// Has custom editor.
        // @function Node:haseditor
        // @return True if the plugin provides it's own editor
        "haseditor",            &Node::hasEditor,

        /// Convert to an XML string.
        // @function Node:toxmlstring
        // @treturn string Node formatted as XML
        "toxmlstring", [](Node* self) -> std::string
        {
            auto copy = self->getValueTree().createCopy();
            Node::sanitizeRuntimeProperties (copy, true);
            return copy.toXmlString().toStdString();
        },

        /// Rebuild port metadata.
        // @function Node:resetports
        "resetports",          &Node::resetPorts,

        /// Save state.
        // @function Node:savestate
        "savestate",           &Node::savePluginState,

        /// Restore state.
        // @function Node:restorestate
        "restorestate",        &Node::restorePluginState,

        /// Write node to file.
        // @function Node:writefile
        // @string f Absolute file path to save to
        // @return True if successful
        "writefile", [](const Node& node, const char* filepath) -> bool {
            if (! File::isAbsolutePath (filepath))
                return false;
            return node.writeToFile (File (String::fromUTF8 (filepath)));
        }
        
       #if 0
        "has_modified_name",    &Node::hasModifiedName,
        "has_node_type",        &Node::hasNodeType,
        "get_parent_graph",     &Node::getParentGraph,
        "is_child_of_root_graph", &Node::isChildOfRootGraph,
        "is_muting_inputs",     &Node::isMutingInputs,
        "set_mute_input",       &Node::setMuteInput,
        "get_num_nodes",        &Node::getNumNodes,
        "get_node",             &Node::getNode,
       #endif
    );

    sol::stack::push (L, kv::lua::remove_and_clear (M, "Node"));
    return 1;
}
