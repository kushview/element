// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

/// A GUI Widget.
// @classmod el.Widget
// @pragma nostrip

#pragma once

#include <element/element.hpp>
#include "sol_helpers.hpp"
#include <element/juce.hpp>

using namespace juce;

namespace element {
namespace lua {

class WidgetProxy
{
public:
    WidgetProxy() = default;
    ~WidgetProxy()
    {
        widget = sol::lua_nil;
        data = nullptr;
    }

    /// Handlers.
    // @section handlers

    /// Called when the widget resizes.
    // @function Widget:resized
    void resized()
    {
        if (sol::safe_function f = widget["resized"])
        {
            f (widget);
        }
    }

    /// Draw your widget here.
    // @function Widget:paint
    // @tparam el.Graphics g The graphics object to paint with
    void paint (Graphics& g)
    {
        if (sol::safe_function f = widget["paint"])
        {
            f (widget, std::ref<Graphics> (g));
        }
    }

    /// Called when the mouse is moving.
    // @function Widget:mouseMove
    // @tparam el.MouseEvent ev The event to process
    void mouseMove (const MouseEvent& ev)
    {
        if (sol::safe_function f = widget["mouseMove"])
            f (widget, ev);
    }

    /// Called when the mouse enters your widget.
    // @function Widget:mouseEnter
    // @tparam el.MouseEvent ev The event to process
    void mouseEnter (const MouseEvent& ev)
    {
        if (sol::safe_function f = widget["mouseEnter"])
            f (widget, ev);
    }

    /// Called when the mouse exits your widget.
    // @function Widget:mouseExit
    // @tparam el.MouseEvent ev The event to process
    void mouseExit (const MouseEvent& ev)
    {
        if (sol::safe_function f = widget["mouseExit"])
            f (widget, ev);
    }

    /// Called when the mouse is dragging.
    // @function Widget:mouseDrag
    // @tparam el.MouseEvent ev The event to process
    void mouseDrag (const MouseEvent& ev)
    {
        if (sol::safe_function f = widget["mouseDrag"])
            f (widget, ev);
    }

    /// Called when the mouse is pressed down.
    // @function Widget:mouseDown
    // @tparam el.MouseEvent ev The event to process
    void mouseDown (const MouseEvent& ev)
    {
        if (sol::safe_function f = widget["mouseDown"])
            f (widget, ev);
    }

    /// Called when the mouse has been released.
    // @function Widget:mouseUp
    // @tparam el.MouseEvent ev The event to process
    void mouseUp (const MouseEvent& ev)
    {
        if (sol::safe_function f = widget["mouseUp"])
            f (widget, ev);
    }

    /// Called when the mouse is double clicked.
    // @function Widget:mouseDoubleClick
    // @tparam el.MouseEvent ev The event to process
    void mouseDoubleClick (const MouseEvent& ev)
    {
        if (sol::safe_function f = widget["mouseDoubleClick"])
            f (widget, ev);
    }

    /// Called when the mouse is double clicked.
    // @function Widget:mouseWheelMove
    // @tparam el.MouseEvent ev The event to process
    // @tparam mixed details Wheel info to process
    void mouseWheelMove (const MouseEvent& ev, const MouseWheelDetails& details)
    {
        if (sol::safe_function f = widget["mouseWheelMove"])
            f (widget, ev, details);
    }

    /// Called when the mouse is double clicked.
    // @function Widget:mouseWheelMove
    // @tparam el.MouseEvent ev The event to process
    // @param  scale The scale to magnify by, 1.0 being no scale
    void mouseMagnify (const MouseEvent& ev, float scale)
    {
        if (sol::safe_function f = widget["mouseMagnify"])
            f (widget, ev, static_cast<lua_Number> (scale));
    }

    sol::table addWithZ (const sol::object& child, int zorder)
    {
        jassert (child.valid());
        if (auto* const w = object_userdata<Component> (widget))
        {
            if (Component* const impl = object_userdata<Component> (child))
            {
                w->addAndMakeVisible (*impl, zorder);
            }
        }
        return child;
    }

    sol::table add (const sol::object& child)
    {
        return addWithZ (child, -1);
    }

    void init (const sol::table& proxy)
    {
        widget = proxy;
        data = object_userdata<Component> (widget);
    }

    sol::table getBoundsTable()
    {
        sol::state_view L (widget.lua_state());
        auto r = data->getBounds();
        auto t = L.create_table();
        t["x"] = r.getX();
        t["y"] = r.getY();
        t["width"] = r.getWidth();
        t["height"] = r.getHeight();
        return t;
    }

    Component* component() noexcept { return data; }

private:
    sol::table widget;
    Component* data = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WidgetProxy)
};

template <typename WidgetType>
static void widget_setbounds (WidgetType& self, const sol::object& obj)
{
    if (obj.is<juce::Rectangle<int>>())
    {
        self.setBounds (obj.as<juce::Rectangle<int>>());
    }
    else if (obj.is<sol::table>())
    {
        sol::table tr = obj;
        self.setBounds (
            tr.get_or ("x", self.getX()),
            tr.get_or ("y", self.getY()),
            tr.get_or ("width", self.getWidth()),
            tr.get_or ("height", self.getHeight()));
    }
}

template <typename Fn>
inline static void setFactoryFunction (const sol::table& T, Fn&& f)
{
    auto T_mt = T[sol::metatable_key];
    T_mt["__newuserdata"] = f;
}

template <typename WidgetType, typename... Args>
inline static sol::table defineWidget (lua_State* L, const char* name, Args&&... args)
{
    // clang-format off
    namespace lua = element::lua;
    using Widget = WidgetType;
    sol::state_view view (L);
    sol::table M = view.create_table();

    M.new_usertype<Widget> (name, sol::no_constructor,
        /// Initialize the widget.
        // Override this to customize your widget. Required as an @{el.object}
        // @function Widget.init
        // @within Class Methods
        "init",         Widget::init,

        /// Attributes.
        // @section attributes

        /// Widget name (string).
        // @field Widget.name
        "name",         sol::property ([] (Widget& self) { return self.getName().toStdString(); }, [] (Widget& self, const char* name) { self.setName (name); }),

        /// X position (readonly).
        // @field Widget.x
        "x",            sol::readonly_property (&Widget::getX),

        /// Y position (readonly).
        // @field Widget.y
        "y",            sol::readonly_property (&Widget::getY),

        /// Widget width (readonly).
        // @field Widget.width
        "width",        sol::readonly_property (&Widget::getWidth),

        /// Widget height (readonly).
        // @field Widget.height
        "height",       sol::readonly_property (&Widget::getHeight),

        /// Widget visibility (bool).
        // Shows or hides this Widget
        // @field Widget.visible
        "visible",      sol::property (&Widget::isVisible, &Widget::setVisible),

        /// Widget is opaque (bool).
        // If set true you must fill the entire Widget background.
        // @field Widget.opaque
        "opaque",       sol::property (&Widget::isOpaque, &Widget::setOpaque),

        /// Methods.
        // @section methods

        /// Returns the bounding box.
        // @function Widget:bounds
        "bounds",       &Widget::getBounds,

        /// Change the bounding box.
        // The coords returned is relative to the top/left of the widget's parent.
        // @function Widget:setBounds
        // @int x X pos
        // @int y Y pos
        // @int w Width
        // @int h Height

        /// Change the bounding box.
        // The coords returned is relative to the top/left of the widget's parent.
        // @function Widget:setBounds
        // @tparam mixed bounds New bounds as a el.Bounds or table
        // @usage
        // -- Can also set a table. e.g.
        // widget:setBounds ({
        //     x      = 0,
        //     y      = 0,
        //     width  = 100,
        //     height = 200
        // })
        "setBounds", sol::overload (
            [] (Widget& self, double x, double y, double w, double h) { 
                self.setBounds (Rectangle<double> (x, y, w, h).toNearestInt()); }, 
            [] (Widget& self, const sol::object& obj) { widget_setbounds (self, obj); }
        ),

        /// Local bounding box.
        // Same as bounds with zero x and y coords
        // @function Widget:localBounds
        "localBounds",      &Widget::getLocalBounds,

        /// Widget right edge.
        // @function Widget:right
        "right",            &Widget::getRight,

        /// Widget bottom edge (int).
        // @function Widget:bottom
        "bottom",           &Widget::getBottom,

        /// Widget Screen X position (int).
        // @function Widget:screenX
        "screenX",          &Widget::getScreenX,

        /// Widget Screen Y position (int).
        // @function Widget:screenY
        "screenY",          &Widget::getScreenY,

        "repaint", sol::overload (
            /// Repaint the entire widget.
            // @function Widget:repaint
            [] (Widget& self) { self.repaint(); },

            /// Repaint a section.
            // @function Widget:repaint
            // @tparam el.Bounds b Area to repaint
            [] (Widget& self, const juce::Rectangle<int>& r) {
                self.repaint (r);
            },
            [] (Widget& self, const juce::Rectangle<double>& r) {
                self.repaint (r.toNearestInt());
            },

            /// Repaint section.
            // @function Widget:repaint
            // @int x
            // @int y
            // @int w
            // @int h
            [] (Widget& self, int x, int y, int w, int h) {
                self.repaint (x, y, w, h);
            }
        ),

        /// Resize the widget.
        // @int w New width
        // @int h New height
        // @function Widget:resize
        "resize",       &Widget::setSize,

        /// Bring to front.
        // @function Widget:toFront
        // @bool focus If true, will also try to focus this widget.
        "toFront",      &Widget::toFront,

        /// To Back.
        // @function Widget:toBack
        "toBack",       &Widget::toBack,

        std::forward<Args> (args)...
        // sol::base_classes,      sol::bases<juce::Component>()
    );

    auto T = lua::removeAndClear (M, name);
    auto T_mt = T [sol::metatable_key];
    T_mt["__newindex"] = sol::lua_nil;

    T_mt["__newuserdata"] = [L]() {
        // sol::state_view view (L);
        // return std::make_unique<WidgetType> (view.create_table());
        return WidgetType::newUserData (L);
    };

    T_mt["__props"] = view.create_table().add (
        "name",
        "x",
        "y",
        "width",
        "height",
        "visible"
        "opaque");

    T_mt["__methods"] = view.create_table().add (
        "bounds",
        "setBounds",
        "localBounds",
        "right",
        "bottom",
        "screenX",
        "screenY",

        "repaint",
        "resize",

        "toFront",
        "toBack"
    );

    view.script (R"(
        require ('el.Bounds')
        require ('el.Graphics')
        require ('el.Point')
        require ('el.Rectangle')
        require ('el.MouseEvent')
    )");

    return T;
    // clang-format on
}

template <typename SliderType, typename... Args>
inline static sol::table registerSlider (lua_State* L, const char* name, Args&&... args)
{
    return defineWidget<SliderType> (L, name, std::forward<Args> (args)...);
}

} // namespace lua
} // namespace element

// clang-format off
#define EL_LUA_IMPLEMENT_WIDGET_PROXY \
protected: \
    element::lua::WidgetProxy proxy; \
public: \
    void paint (juce::Graphics& g) override { proxy.paint (g); } \
    void resized() override { proxy.resized(); } \
    void mouseMove (const MouseEvent& ev) override { proxy.mouseMove (ev); } \
    void mouseEnter (const MouseEvent& ev) override { proxy.mouseEnter (ev); } \
    void mouseExit (const MouseEvent& ev) override { proxy.mouseExit (ev); } \
    void mouseDrag (const MouseEvent& ev) override { proxy.mouseDrag (ev); } \
    void mouseDown (const MouseEvent& ev) override { proxy.mouseDown (ev); } \
    void mouseUp (const MouseEvent& ev) override { proxy.mouseUp (ev); } \
    void mouseDoubleClick (const MouseEvent& ev) override { proxy.mouseDoubleClick (ev); } \
    void mouseWheelMove (const MouseEvent& ev, const MouseWheelDetails& details) override { proxy.mouseWheelMove (ev, details); } \
    void mouseMagnify (const MouseEvent& ev, float scale) override { proxy.mouseMagnify (ev, scale); }
// clang-format on
