/// Desktop controls.
// Describes and controls aspects of the computer's desktop.
// @classmod el.Desktop
// @pragma nostrip

#include <element/element.hpp>
#include <element/juce/gui_basics.hpp>

#include "sol_helpers.hpp"

#define LKV_TYPE_NAME_DESKTOP "Desktop"

using namespace juce;

EL_PLUGIN_EXPORT
int luaopen_el_Desktop (lua_State* L)
{
    sol::state_view lua (L);
    auto M = lua.create_table();
    M.new_usertype<Desktop> (LKV_TYPE_NAME_DESKTOP, sol::no_constructor,
                             /// Desktop single instance.
                             // @function Desktop.instance
                             // @within Class Methods
                             "instance",
                             Desktop::getInstance,

                             /// Global scale factor.
                             // Sets a global scale factor to be used for all desktop windows.
                             // Setting this will also scale display sizes.
                             // @tfield number Desktop.scale
                             // @within Attributes
                             "scale",
                             sol::property (&Desktop::getGlobalScaleFactor, &Desktop::setGlobalScaleFactor));

    sol::stack::push (L, element::lua::remove_and_clear (M, LKV_TYPE_NAME_DESKTOP));
    return 1;
}
