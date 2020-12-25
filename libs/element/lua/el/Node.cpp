/// Node object.
// @classmod el.Node
// @pragma nostrip

#include "lua-kv.hpp"
#include "session/Node.h"

LUAMOD_API int luaopen_el_Node (lua_State* L) {
    using namespace Element;
    using namespace sol;

    sol::state_view lua (L);
    auto M = lua.create_table();
    M.new_usertype<Node> ("Node", no_constructor,
        meta_function::to_string, [](const Node& self) -> std::string {
            String str = self.isGraph() ? "Graph" : "Node";
            if (self.getName().isNotEmpty())
                str << ": " << self.getName();
            return std::move (str.toStdString());
        },
        meta_function::length,  &Node::getNumNodes,
        meta_function::index,   [](Node* self, int index)
        {
            const auto child = self->getNode (index - 1);
            return child.isValid() ? std::make_shared<Node> (child.getValueTree(), false)
                                   : std::shared_ptr<Node>();
        },

        /// True if a valid node (readonly).
        // @field Node.valid
        // @within Attributes
        "valid",                readonly_property (&Node::isValid),

        /// Node name.
        // @field Node.name
        // @within Attributes
        "name", property (
            [](Node* self) { return self->getName().toStdString(); },
            [](Node* self, const char* name) { self->setProperty (Tags::name, String::fromUTF8 (name)); }
        ),
        
        /// Is the node missing?.
        // @field Node.missing
        // @within Attributes
        "missing",              readonly_property (&Node::isMissing),

        /// True if enabled.
        // @field Node.enabled
        // @within Attributes
        "enabled",              readonly_property (&Node::isEnabled),

        "uuid",                 readonly_property (&Node::getUuid),
        "uuidstring",           readonly_property (&Node::getUuidString),
        
        "graph",                readonly_property (&Node::isGraph),
        "root",                 readonly_property (&Node::isRootGraph),
        "nodeid",               readonly_property (&Node::getNodeId),
        
        "type",                 readonly_property (&Node::getNodeType),
        "muted",                property (&Node::isMuted, &Node::setMuted),
        "bypassed",             readonly_property (&Node::isBypassed),
        "has_editor",           readonly_property (&Node::hasEditor),

        "display_name",         [](Node* self) { return self->getDisplayName().toStdString(); },
        "plugin_name",          [](Node* self) { return self->getPluginName().toStdString(); },

        "toxmlstring", [](Node* self) -> std::string
        {
            auto copy = self->getValueTree().createCopy();
            Node::sanitizeRuntimeProperties (copy, true);
            return copy.toXmlString().toStdString();
        },

        "reset_ports",          &Node::resetPorts,
        "save_state",           &Node::savePluginState,
        "restore_state",        &Node::restorePluginState,
        "write_file", [](const Node& node, const char* filepath) -> bool {
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
