// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

/// An Element View.
// Is a @{el.Widget}
// @classmod el.View
// @pragma nostrip

#include <element/element.h>
#include <element/ui/view.hpp>
#include "object.hpp"
#include "widget.hpp"

#define EL_TYPE_NAME_VIEW "View"

namespace element {
namespace lua {

class View : public element::View
{
public:
    View (const sol::table&) {}
    ~View() {}

    static void init (const sol::table& view)
    {
        if (auto* impl = object_userdata<lua::View> (view))
        {
            impl->view = view;
            impl->initialize();
        }
    }

    static auto newUserData (lua_State* L)
    {
        juce::ignoreUnused (L);
        return std::make_unique<lua::View> (sol::table());
    }

    void initialize()
    {
        proxy.init (view);
    }

    sol::table addWithZ (const sol::object& child, int zorder)
    {
        return proxy.addWithZ (child, zorder);
    }

    sol::table add (const sol::object& child)
    {
        return proxy.add (child);
    }

private:
    sol::table view;
    EL_LUA_IMPLEMENT_WIDGET_PROXY
};

} // namespace lua
} // namespace element

EL_PLUGIN_EXPORT
int luaopen_el_View (lua_State* L)
{
    // clang-format off
    using namespace juce;
    using element::lua::View;
    namespace lua = element::lua;

    auto T = lua::defineWidget<View> (
        L, EL_TYPE_NAME_VIEW, sol::meta_method::to_string, [] (View& self) { 
            return lua::to_string (self, EL_TYPE_NAME_VIEW); 
        },

        /// Add a child widget to this view.
        "add", sol::overload (&View::add, &View::addWithZ),
        sol::base_classes, sol::bases<juce::Component>()
    );

    lua::Object<View>::addMethods (T, "add");
    sol::stack::push (L, T);
    return 1;
}
