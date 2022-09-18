/// A drawing context.
// @classmod el.Graphics
// @pragma nostrip

#include "lua-kv.hpp"
#include "sol_helpers.hpp"

using namespace juce;

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
        // @function Graphics:savestate
        "savestate",
        &Graphics::saveState,

        /// Restore the last saved state.
        // @function Graphics:restorestate
        "restorestate",
        &Graphics::restoreState,

        /// Change the color.
        // @function Graphics:setcolor
        // @int color New ARGB color as integer. e.g.`0xAARRGGBB`
        "setcolor",
        [] (Graphics& g, int color) { g.setColour (Colour (color)); },

        /// Draw some text.
        // @function Graphics:drawtext
        // @string text Text to draw
        // @tparam el.Rectangle r

        /// Draw some text.
        // @function Graphics:drawtext
        // @string text Text to draw
        // @int x Horizontal position
        // @int y Vertical position
        // @int w Width of containing area
        // @int h Height of containing area
        "drawtext",
        sol::overload ([] (Graphics& g, const char* text, Rectangle<double> r) { g.drawText (text, r.toFloat(), Justification::centred, true); }, [] (Graphics& g, std::string t, int x, int y, int w, int h) { g.drawText (t, x, y, w, h, Justification::centred, true); }),

        /// Fill the entire drawing area.
        // Fills the drawing area with the current color.
        // @function Graphics:fillall
        "fillall",
        sol::overload ([] (Graphics& g) { g.fillAll(); }, [] (Graphics& g, int color) { g.fillAll (Colour (color)); }));
    sol::stack::push (L, element::lua::remove_and_clear (M, "Graphics"));
    return 1;
}
