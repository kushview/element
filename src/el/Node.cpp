
#include <element/element.h>
#include <element/node.hpp>
#include "./nodetype.hpp"

// clang-format off

EL_PLUGIN_EXPORT int luaopen_el_Node (lua_State* L)
{
    using namespace element;
    using namespace sol;
    auto M = element::lua::new_nodetype<element::Node> (L, "Node");
#if 0
    sol::state_view lua (L);
    auto M = lua.create_table();
    M.new_usertype<Node> ("Node", no_constructor, 
        meta_function::to_string, [] (const Node& self) -> std::string {
            String str = self.isGraph() ? "Graph" : "Node";
            if (self.getName().isNotEmpty())
                str << ": " << self.getName();
            return str.toStdString(); 
        },
        meta_function::length, &Node::getNumNodes,
        meta_function::index, [] (Node* self, int index) {
            const auto child = self->getNode (index - 1);
            return child.isValid() ? std::make_shared<Node> (child.getValueTree(), false)
                                    : std::shared_ptr<Node>(); 
        },

        "name",        [] (Node* self) { return self->getName().toStdString(); }, 
        "setName",     [] (Node* self, const char* name) {  self->setProperty (Tags::name, String::fromUTF8 (name)); }),
        "displayName", [] (Node* self) { return self->getDisplayName().toStdString(); },
        "pluginName",  [] (Node* self) { return self->getPluginName().toStdString(); },
        "hasModifiedName", &Node::hasModifiedName,

        "uuid",       [](Node& self) { return self.getUuidString().toStdString(); },
        "nodeId",     [](Node& self) { return static_cast<lua_Integer> (self.getNodeId()); },
        "nodeType",   [](Node& self) { return self.getNodeType().toString(); },

        "isMissing",   &Node::isMissing,
        "isValid",     &Node::isValid,
        "isEnabled",   &Node::isEnabled,
        "isBypased",   &Node::isBypassed,
        "isMuted",     &Node::isMuted,
        "isGraph",     &Node::isGraph,
        "isRootGraph", &Node::isRootGraph,

        "hasEditor",   &Node::hasEditor,

        "toXmlString", [] (Node* self) -> std::string {
            auto copy = self->getValueTree().createCopy();
            Node::sanitizeRuntimeProperties (copy, true);
            return copy.toXmlString().toStdString();
        },
        "writeFile", [] (const Node& node, const char* filepath) -> bool {
            if (! File::isAbsolutePath (filepath))
                return false;
            return node.writeToFile (File (String::fromUTF8 (filepath)));
        },

        "resetPorts",   &Node::resetPorts,

        "saveState",    &Node::savePluginState,
        "restoreState", &Node::restorePluginState

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
#endif

    sol::stack::push (L, element::lua::remove_and_clear (M, "Node"));
    return 1;
}
