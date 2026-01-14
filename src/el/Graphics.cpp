// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

/// A drawing context.
// @classmod el.Graphics
// @pragma nostrip

#include <element/juce/graphics.hpp>
#include <element/element.hpp>
#include "sol_helpers.hpp"

using namespace juce;
namespace lua = element::lua;

// clang-format off
EL_PLUGIN_EXPORT
int luaopen_el_Graphics (lua_State* L)
{
    sol::state_view lua (L);

    auto M = lua.create_table();
    M.new_usertype<Graphics> (
        "Graphics", sol::no_constructor,
        /// Methods.
        // @section methods

        /// Save the current state.
        // @function Graphics:save
        "save",     &Graphics::saveState,

        /// Restore the last saved state.
        // @function Graphics:restore
        "restore",  &Graphics::restoreState,

        /// Change the color.
        // @function Graphics:setColor
        // @int color New ARGB color as integer. e.g.`0xAARRGGBB`
        "setColor", [] (Graphics& g, lua_Integer color) { 
            g.setColour (Colour (color)); 
        },

        /// Draw some text.
        // @function Graphics:drawText
        // @string text Text to draw
        // @tparam el.Rectangle r

        /// Draw some text.
        // @function Graphics:drawText
        // @string text Text to draw
        // @int x Horizontal position
        // @int y Vertical position
        // @int w Width of containing area
        // @int h Height of containing area
        "drawText", sol::overload (
            [] (Graphics& g, const char* text, Rectangle<int> r) { 
                g.drawText (text, r.toFloat(), Justification::centred, true); 
            },
            [] (Graphics& g, const char* text, Rectangle<double> r) { 
                g.drawText (text, r.toFloat(), Justification::centred, true); 
            },
            [] (Graphics& g, const char* text, Rectangle<float> r) { 
                g.drawText (text, r, Justification::centred, true); 
            }, 
            [] (Graphics& g, std::string t, int x, int y, int w, int h) { 
                g.drawText (t, x, y, w, h, Justification::centred, true); 
            }
        ),

        /// Fill the entire drawing area.
        // Fills the drawing area with the current color.
        // @function Graphics:fillall
        "fillAll", sol::overload (
            [] (Graphics& g) { g.fillAll(); }, 
            [] (Graphics& g, int color) { g.fillAll (Colour (color)); })
        );
    sol::stack::push (L, lua::removeAndClear (M, "Graphics"));
    return 1;
}
