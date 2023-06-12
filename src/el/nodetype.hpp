
#pragma once

#include "sol_helpers.hpp"

namespace element {
namespace lua {
    template <typename NT, typename... Args>
    inline static sol::table new_nodetype (lua_State* L, const char* name, Args&&... args)
    {
        using namespace element;
        using namespace sol;
        using Node = NT;
        sol::state_view lua (L);
        auto M = lua.create_table();
        M.new_usertype<Node> (name, no_constructor, 
            meta_function::to_string, lua::to_string, 
            meta_function::length, &Node::getNumNodes,
            meta_function::index, [] (Node* self, int index) {
                const auto child = self->getNode (index - 1);
                return child.isValid() ? std::make_shared<Node> (child.getValueTree(), false)
                                    : std::shared_ptr<Node>(); 
            },

            "name",        [] (Node* self) {
                return self->getName().toStdString(); }, 
            "setName",     [] (Node* self, const char* name) {
                self->setProperty (Tags::name, String::fromUTF8 (name)); }),
            "displayName", [] (Node* self) {
                return self->getDisplayName().toStdString(); },
            "pluginName",  [] (Node* self) {
                return self->getPluginName().toStdString(); },
            "hasModifiedName", &Node::hasModifiedName,

            "uuid",       [](Node& self) {
                return self.getUuidString().toStdString(); },
            "nodeId",     [](Node& self) {
                return static_cast<lua_Integer> (self.getNodeId()); },
            "nodeType",   [](Node& self) {
                return self.getNodeType().toString(); },

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
            "restoreState", &Node::restorePluginState,
            std::forward<Args> (args)...
        );
        // clang-format on
    }

} // namespace lua
} // namespace element
