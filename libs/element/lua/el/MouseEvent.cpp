/// A mouse event
// @classmod kv.MouseEvent
// @pragma nostrip

#include "lua-kv.hpp"
#include LKV_JUCE_HEADER

#define LKV_TYPE_NAME_MOUSE_EVENT "MouseEvent"

using namespace juce;

LKV_EXPORT
int luaopen_el_MouseEvent (lua_State* L)
{
    sol::state_view lua (L);
    auto M = lua.create_table();
    M.new_usertype<MouseEvent> (LKV_TYPE_NAME_MOUSE_EVENT, sol::no_constructor,
        /// Attributes.
        // @section attributes

        /// Position as @{kv.Point}.
        // @field MouseEvent.position
        "position", sol::readonly_property ([](MouseEvent& self) {
            return self.position.toFloat();
        }),

        /// X position.
        // @tfield int MouseEvent.x
        "x",            &MouseEvent::x,

        /// Y position.
        // @tfield int MouseEvent.y
        "y",            &MouseEvent::y,

        /// Pressure (0.0-1.0).
        // @tfield number MouseEvent.pressure
        "pressure",     &MouseEvent::pressure,

        /// Orientation in radians.
        // @tfield number MouseEvent.orientation
        "orientation",  &MouseEvent::orientation,

        /// Rotation in radians.
        // @tfield number MouseEvent.rotation
        "rotation",     &MouseEvent::rotation,

        /// Tilt X position (-1.0-1.0).
        // @tfield number MouseEvent.tiltx
        "tiltx",        &MouseEvent::tiltX,

        /// Tilt Y (-1.0-1.0).
        // @tfield number MouseEvent.tilty
        "tilty",        &MouseEvent::tiltY
    );

    sol::stack::push (L, kv::lua::remove_and_clear (M, LKV_TYPE_NAME_MOUSE_EVENT));
    return 1;
}
