// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

/// A GUI Widget.
//
// Is defined with @{el.object} and can be inherrited.  Objects must be
// instantiateed using @{object.new}
// @classmod el.Widget
// @pragma nostrip

#include <element/element.h>
#include "object.hpp"
#include "widget.hpp"
#define EL_TYPE_NAME_WIDGET "Widget"

using namespace juce;

namespace element {
namespace lua {

class Widget : public Component
{
public:
    Widget (const sol::table& t) {}
    static void init (const sol::table& t)
    {
        if (auto impl = object_userdata<Widget> (t))
        {
            (*impl).proxy.init (t);
        }
    }

    static auto newUserData (lua_State* L)
    {
        juce::ignoreUnused (L);
        return std::make_unique<Widget> (sol::table());
    }

    sol::table addWithZ (const sol::object& child, int zorder)
    {
        jassert (child.valid());
        if (auto* const w = proxy.component())
        {
            if (Component* const impl = object_userdata<Component> (child))
            {
                w->addAndMakeVisible (*impl, zorder);
            }
            else
            {
                std::clog << "[element::lua] child is not a component." << std::endl;
            }
        }
        else
        {
            std::clog << "[element::lua] proxy has no component." << std::endl;
        }

        return child;
    }

    sol::table add (const sol::object& child)
    {
        return addWithZ (child, -1);
    }

    /** Proxy */
    EL_LUA_IMPLEMENT_WIDGET_PROXY
};

} // namespace lua
} // namespace element

// clang-format off
EL_PLUGIN_EXPORT
int luaopen_el_Widget (lua_State* L)
{
    namespace lua = element::lua;
    using Widget = element::lua::Widget;

    auto T = lua::defineWidget<Widget> (
        L, EL_TYPE_NAME_WIDGET, sol::meta_method::to_string, [] (Widget& self) {
            return lua::to_string (self, EL_TYPE_NAME_WIDGET);
        },
        /// Add a child widget.
        // @function Widget:add
        // @tparam el.Widget widget Widget to add
        // @int[opt] zorder Z-order
        // @within Methods
        "add", sol::overload (&Widget::add, &Widget::addWithZ),

        /// Elevate this widget to toplevel status.
        // @function Widget:elevate
        // @within Methods
        "elevate", sol::overload (
            [] (Widget& self, int flags) { 
                self.addToDesktop (flags, nullptr);
            }, 
            [] (Widget& self, int flags, void* handle) { 
                self.addToDesktop (flags, handle); 
            }
        ),

        sol::base_classes, sol::bases<juce::Component>()
    );

    lua::Object<Widget>::addMethods (T, "add", "elevate");
    sol::stack::push (L, T);

    return 1;
}
