
#pragma once

#include "sol_helpers.hpp"

namespace element {
namespace lua {
// clang-format off
template <typename NT, typename... Args>
inline static sol::table new_nodetype (lua_State* L, const char* name, Args&&... args)
{
    using namespace element;
    using namespace sol;
    using Node = NT;
    sol::state_view lua (L);
    auto M = lua.create_table();
    M.new_usertype<Node> (name, no_constructor, 
        meta_function::length, &Node::getNumNodes,
        meta_function::index, [] (Node* self, int index) {
            const auto child = self->getNode (index - 1);
            return child.isValid() ? std::make_shared<Node> (child.data(), false)
                                    : std::shared_ptr<Node>(); 
        },

        "name",        [] (Node* self) { return self->getName().toStdString(); }, 
        "setName",     [] (Node* self, const char* name) { self->setProperty (tags::name, String::fromUTF8 (name)); },
        "displayName", [] (Node* self) { return self->getDisplayName().toStdString(); },
        "pluginName",  [] (Node* self) { return self->getPluginName().toStdString(); },
        "hasModifiedName", &Node::hasModifiedName,
        "uuidString",  [](Node& self) { return self.getUuidString().toStdString(); },
        "nodeId",      [](Node& self) { return static_cast<lua_Integer> (self.getNodeId()); },
        "nodeType",    [](Node& self) { return self.getNodeType().toString(); },

        "missing",     &Node::isMissing,
        "valid",       &Node::isValid,
        "enabled",     &Node::isEnabled,
        "bypassed",    &Node::isBypassed,
        "muted",       &Node::isMuted,
        "isGraph",     &Node::isGraph,
        "isRoot",      &Node::isRootGraph,

        "toXmlString", [] (Node* self) -> std::string {
            auto copy = self->data().createCopy();
            element::Node::sanitizeRuntimeProperties (copy, true);
            return copy.toXmlString().toStdString();
        },
        "writeFile", [] (const Node& node, const char* filepath) -> bool {
            if (! File::isAbsolutePath (filepath))
                return false;
            return node.writeToFile (File (String::fromUTF8 (filepath)));
        },

        "resetPorts",   &Node::resetPorts,

        "saveState",    &Node::savePluginState,
        "restoreState", &Node::restorePluginState,
        std::forward<Args> (args)...
    );
    return remove_and_clear (M, name);
}
// clang-format off

} // namespace lua
} // namespace element
