/// Node object.
// @classmod el.Node
// @pragma nostrip

#include <element/element.h>
#include <element/node.hpp>
#include "sol_helpers.hpp"

EL_PLUGIN_EXPORT int luaopen_el_Node (lua_State* L)
{
    using namespace element;
    using namespace sol;

    sol::state_view lua (L);
    auto M = lua.create_table();
    M.new_usertype<Node> (
        "Node", no_constructor, meta_function::to_string, [] (const Node& self) -> std::string {
            String str = self.isGraph() ? "Graph" : "Node";
            if (self.getName().isNotEmpty())
                str << ": " << self.getName();
            return str.toStdString(); }, meta_function::length, &Node::getNumNodes, meta_function::index, [] (Node* self, int index) {
            const auto child = self->getNode (index - 1);
            return child.isValid() ? std::make_shared<Node> (child.getValueTree(), false)
                                   : std::shared_ptr<Node>(); },
        // clang-format off

        /// Attributes.
        // @section Attributes

        /// Node name.
        // @field Node.name
        // @within Attributes
        "name",
        property ([] (Node* self) { return self->getName().toStdString(); }, 
                  [] (Node* self, const char* name) { self->setProperty (Tags::name, String::fromUTF8 (name)); }),

        "missing",
        readonly_property (&Node::isMissing),
        "uuid",
        readonly_property (&Node::getUuid),
        "uuidstring",
        readonly_property (&Node::getUuidString),
        "nodeid",
        readonly_property (&Node::getNodeId),
        "type",
        readonly_property (&Node::getNodeType),

        /// Methods.
        // @section methods

        /// Returns true if a valid node.
        // @function Node:valid
        // @within Methods
        "valid",
        &Node::isValid,

        /// True if enabled.
        // @function Node:enabled
        // @within Methods
        "enabled",
        &Node::isEnabled,

        /// True if bypassed.
        // @function Node:isBypassed
        "isBypassed",
        &Node::isBypassed,

        /// True if muted.
        // @function Node:isMuted
        "isMuted",
        &Node::isMuted,

        /// Display name.
        // Will be a user-defined name or name provided by the plugin.
        // @function Node:displayName
        // @treturn string
        "displayName",
        [] (Node* self) { return self->getDisplayName().toStdString(); },

        /// Plugin name.
        // Name provided by the plugin.
        // @function Node:pluginName
        // @treturn string
        "pluginName",
        [] (Node* self) { return self->getPluginName().toStdString(); },

        /// True if this node is a graph.
        // @function Node:isGraph
        "isGraph",
        &Node::isGraph,

        /// True if this node is a root graph.
        // @function Node:isRoot
        "isRoot",
        &Node::isRootGraph,

        /// Has custom editor.
        // @function Node:hasEditor
        // @return True if the plugin provides it's own editor
        "hasEditor",
        &Node::hasEditor,

        /// Convert to an XML string.
        // @function Node:toXmlString
        // @treturn string Node formatted as XML
        "toXmlString",
        [] (Node* self) -> std::string {
            auto copy = self->getValueTree().createCopy();
            Node::sanitizeRuntimeProperties (copy, true);
            return copy.toXmlString().toStdString();
        },

        /// Rebuild port metadata.
        // @function Node:resetPorts
        "resetPorts",
        &Node::resetPorts,

        /// Save state.
        // @function Node:save
        "save",
        &Node::savePluginState,

        /// Restore state.
        // @function Node:restore
        "restore",
        &Node::restorePluginState,

        /// Write node to file.
        // @function Node:writefile
        // @string f Absolute file path to save to
        // @return True if successful
        "writeFile",
        [] (const Node& node, const char* filepath) -> bool {
            if (! File::isAbsolutePath (filepath))
                return false;
            return node.writeToFile (File (String::fromUTF8 (filepath)));
        },

        /// Returns true is the node name has changed from default.
        // @function Node:hasModifiedName
        // @return True if modified
        "hasModifiedName",
        &Node::hasModifiedName
#if 0
        "has_node_type",        &Node::hasNodeType,
        "get_parent_graph",     &Node::getParentGraph,
        "is_child_of_root_graph", &Node::isChildOfRootGraph,
        "is_muting_inputs",     &Node::isMutingInputs,
        "set_mute_input",       &Node::setMuteInput,
        "get_num_nodes",        &Node::getNumNodes,
        "get_node",             &Node::getNode,
#endif
        // clang-format on
    );

    sol::stack::push (L, element::lua::remove_and_clear (M, "Node"));
    return 1;
}
