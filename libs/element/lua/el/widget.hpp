/// A GUI Widget.
// @classmod kv.Widget
// @pragma nostrip

#pragma once
#include "lua-kv.hpp"
#include LKV_JUCE_HEADER

namespace kv {
namespace lua {

template<typename WidgetType>
static void widget_setbounds (WidgetType& self, const sol::object& obj) {
    if (obj.is<juce::Rectangle<int>>())
    {
        self.setBounds (obj.as<juce::Rectangle<int>>());
    }
    else if (obj.is<sol::table>())
    {
        sol::table tr = obj;
        self.setBounds (
            tr.get_or ("x",      self.getX()),
            tr.get_or ("y",      self.getY()),
            tr.get_or ("width",  self.getWidth()),
            tr.get_or ("height", self.getHeight())
        );
    }
}

template<typename WidgetType, typename ...Args>
inline static sol::table
new_widgettype (lua_State* L, const char* name, Args&& ...args) {
    using Widget = WidgetType;
    sol::state_view lua (L);
    sol::table M = lua.create_table();

    M.new_usertype<Widget> (name, sol::no_constructor,
        /// Initialize the widget.
        // Override this to customize your widget.
        // @function Widget.init
        "init", Widget::init,

        /// Attributes.
        // @section attributes

        /// Widget name (string).
        // @field Widget.name
        "name", sol::property (
            [](Widget& self) { return self.getName().toStdString(); },
            [](Widget& self, const char* name) { self.setName (name); }
        ),

        /// X position (readonly).
        // @field Widget.x
        "x",                    sol::readonly_property (&Widget::getX),

        /// Y position (readonly).
        // @field Widget.y
        "y",                    sol::readonly_property (&Widget::getY),

        /// Widget width (readonly).
        // @field Widget.width
        "width",                sol::readonly_property (&Widget::getWidth),

        /// Widget height (readonly).
        // @field Widget.height
        "height",               sol::readonly_property (&Widget::getHeight),

        /// Widget visibility (bool).
        // Shows or hides this Widget
        // @field Widget.visible
        "visible",              sol::property (&Widget::isVisible, &Widget::setVisible),
        
        /// Widget is opaque (bool).
        // If set true you must fill the entire Widget background.
        // @field Widget.opaque
        "opaque",               sol::property (&Widget::isOpaque, &Widget::setOpaque),

        /// Methods.
        // @section methods

        /// Returns the bounding box.
        // @function Widget:bounds
        "bounds",               &Widget::getBounds,

        /// Change the bounding box.
        // The coords returned is relative to the top/left of the widget's parent.
        // @function Widget:setbounds
        // @int x X pos
        // @int y Y pos
        // @int w Width
        // @int h Height

        /// Change the bounding box.
        // The coords returned is relative to the top/left of the widget's parent.
        // @function Widget:setbounds
        // @tparam mixed New bounds as a kv.Bounds or table
        // @usage
        // -- Can also set a table. e.g.
        // widget:setbounds ({
        //     x      = 0, 
        //     y      = 0,
        //     width  = 100,
        //     height = 200
        // })
        "setbounds", sol::overload (
            [](Widget& self, double x, double y, double w, double h) {
                self.setBounds (x, y, w, h);
            },
            [](Widget& self, const sol::object& obj) {
                widget_setbounds (self, obj);
            }
        ),

        /// Local bounding box.
        // Same as bounds with zero x and y coords
        // @function Widget:localbounds
        "localbounds",          &Widget::getLocalBounds,

        /// Widget right edge.
        // @function Widget:right
        "right",                &Widget::getRight,

        /// Widget bottom edge (int).
        // @function Widget:bottom
        "bottom",               &Widget::getBottom,

        /// Widget Screen X position (int).
        // @function Widget:screenx
        "screenx",              &Widget::getScreenX,

        /// Widget Screen Y position (int).
        // @function Widget:screeny
        "screeny",              &Widget::getScreenY,

        "repaint",  sol::overload (
            /// Repaint the entire widget.
            // @function Widget:repaint
            [](Widget& self) { self.repaint(); },

            /// Repaint a section.
            // @function Widget:repaint
            // @tparam kv.Bounds b Area to repaint
            [](Widget& self, const juce::Rectangle<int>& r) { 
                self.repaint (r); 
            },
            [](Widget& self, const juce::Rectangle<double>& r) { 
                self.repaint (r.toNearestInt());
            },

            /// Repaint section.
            // @function Widget:repaint
            // @int x
            // @int y
            // @int w
            // @int h
            [](Widget& self, int x, int y, int w, int h) { 
                self.repaint (x, y, w, h); 
            }
        ),

        /// Resize the widget.
        // @int w New width
        // @int h New height
        // @function Widget:resize
        "resize",               &Widget::setSize,
        
        /// Bring to front.
        // @function Widget:tofront
        // @bool focus If true, will also try to focus this widget.
        "tofront",              &Widget::toFront,

        /// To Back.
        // @function Widget:toback
        "toback",               &Widget::toBack,

        /// Remove from desktop.
        // @function Widget:removefromdesktop
        "removefromdesktop",    &Widget::removeFromDesktop,

        // Makes this widget appear as a window on the desktop.
        //
        // Note that before calling this, you should make sure that the widget's opacity is
        // set correctly using setOpaque(). If the widget is non-opaque, the windowing
        // system will try to create a special transparent window for it, which will generally take
        // a lot more CPU to operate (and might not even be possible on some platforms).
        //
        // If the widget is inside a parent widget at the time this method is called, it
        // will first be removed from that parent. Likewise if a widget is on the desktop
        // and is subsequently added to another widget, it'll be removed from the desktop.
        //
        // @function Widget:addtodesktop
        // @int flags Window flags
        // @param[opt] window Native window handle to attach to
        
        // TODO: addtodesktop

        /// True if the widget is showing on the desktop.
        // @function Widget:isondesktop
        "isondesktop",          &Widget::isOnDesktop,
        
        std::forward <Args> (args)...
        // sol::base_classes,      sol::bases<juce::Component>()
    );

    auto T = kv::lua::remove_and_clear (M, name);
    auto T_mt = T[sol::metatable_key];
    T_mt["__newindex"] = sol::lua_nil;
    T_mt["__newuserdata"] = [L]() {
        sol::state_view view (L);
        return std::make_unique<Widget> (view.create_table());
    };

    T_mt["__props"] = lua.create_table().add (
        "name",
        "x",
        "y",
        "width", 
        "height", 
        "visible"
        "opaque"
    );
    
    T_mt["__methods"] = lua.create_table().add (
        "bounds",
        "setbounds",
        "localbounds",
        "right",
        "bottom",
        "screenx", 
        "screeny",

        "repaint",
        "resize",
        "tofront",
        "toback",

        "addtodesktop",
        "removefromdesktop",
        "isondesktop"        
    );

    lua.script (R"(
        require ('kv.Bounds')
        require ('kv.Graphics')
        require ('kv.Point')
        require ('kv.Rectangle')
    )");

    return T;
}

template<typename SliderType, typename ...Args>
inline static sol::table
new_slidertype (lua_State* L, const char* name, Args&& ...args) {
    return new_widgettype<SliderType> (name,
    std::forward<Args> (args)...);
}

}}
