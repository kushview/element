// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

/// The base graph editor widget.
// Is a @{el.Widget}
// @classmod el.GraphEditor
// @pragma nostrip

#include <element/element.h>
#include "ui/grapheditorcomponent.hpp"
#include "object.hpp"
#include "widget.hpp"

#define EL_TYPE_NAME_VIEW "GraphEditor"

namespace element {
namespace lua {

class GraphEditor : public element::GraphEditorComponent
{
public:
    GraphEditor() {}
    ~GraphEditor() {}

    static void init (const sol::table& view)
    {
        if (auto* impl = object_userdata<lua::GraphEditor> (view))
        {
            impl->view = view;
            impl->initialize();
        }
    }

    static auto newUserData (lua_State* L)
    {
        juce::ignoreUnused (L);
        return std::make_unique<lua::GraphEditor>();
    }

    void initialize()
    {
        // proxy.init (view);
    }

private:
    sol::table view;
    // EL_LUA_IMPLEMENT_WIDGET_PROXY
};

} // namespace lua
} // namespace element

EL_PLUGIN_EXPORT
int luaopen_el_GraphEditor (lua_State* L)
{
    // clang-format off
    using namespace juce;
    using element::lua::GraphEditor;
    namespace lua = element::lua;

    auto T = lua::defineWidget<GraphEditor> (
        L, EL_TYPE_NAME_VIEW, sol::meta_method::to_string, [] (GraphEditor& self) { 
            return lua::to_string (self, EL_TYPE_NAME_VIEW); 
        },

        /// The el.Graph used by this editor.
        // @field GraphEditor.graph
        // @within Attributes
        "graph", sol::property (&GraphEditor::getGraph, &GraphEditor::setNode),
        sol::base_classes, sol::bases<juce::Component>()
    );

    // lua::Object<GraphEditor>::addMethods (T, "setGraph");
    lua::Object<GraphEditor>::exportAttributes (T, "graph");

    sol::state_view view (L);
    view.script (R"(
        require ('el.Node')
    )");

    sol::stack::push (L, T);

    return 1;
}
