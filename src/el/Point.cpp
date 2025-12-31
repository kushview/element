// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

/// A pair of x,y coordinates.
// @classmod el.Point
// @pragma nostrip

#include <element/element.h>
#include <element/juce/graphics.hpp>

#include "sol_helpers.hpp"
#define EL_TYPE_NAME_POINT "Point"

using namespace juce;

int register_el_Point (lua_State* L)
{
    sol::state_view lua (L);
    using PTF = Point<lua_Number>;
    auto M = lua.create_table();
    M.new_usertype<PTF> (
        EL_TYPE_NAME_POINT, sol::no_constructor, "new", sol::factories (
                                                            /// Create a new point with x and y = 0.
                                                            // @function Point.new
                                                            // @treturn el.Point
                                                            // @within Constructors
                                                            []() { return PTF(); },

                                                            /// Create a new point.
                                                            // @function Point.new
                                                            // @number x X coordinate
                                                            // @number y Y coordinate
                                                            // @treturn el.Point
                                                            // @within Constructors
                                                            [] (lua_Number x, lua_Number y) { return PTF (x, y); }),
        sol::meta_method::to_string, [] (PTF& self) {
            return self.toString().toStdString();
        },

        /// X coord (number)
        // @class field
        // @name Point.x
        // @within Attributes
        "x", sol::property (&PTF::getX, &PTF::setX),

        /// Y coord (number)
        // @class field
        // @name Point.x
        // @within Attributes
        "y", sol::property (&PTF::getY, &PTF::setY),

        /// Methods.
        // @section methods

        /// True if is the origin point
        // @function Point:isOrigin
        // @within Methods
        "isOrigin",  &PTF::isOrigin,

        /// True if is finite
        // @function Point:isFinite
        // @within Methods
        "isFinite", &PTF::isFinite,

        /// Returns a point with the given x coordinate.
        // @param x New x coord
        // @function Point:withX
        // @treturn el.Point New point object
        "withX", &PTF::withX,

        /// Returns a point with the given y coordinate.
        // @param y New y coord
        // @function Point:withY
        // @treturn el.Point New point object
        "withY", &PTF::withY,

        /// Set x and y at the same time.
        // @number x New x coordinate
        // @number y New y coordinate
        // @function Point:setXY
        "setXY",  &PTF::setXY,

        /// Adds a pair of coordinates to this value.
        // @number x X to add
        // @number y Y to add
        // @function Point:addXY
        "addXY",  &PTF::addXY,

        /// Move the point by delta x and y.
        // @function Point:translated
        // @number deltax
        // @number deltay
        "translated", &PTF::translated,

        /// Distance to another point.
        // @function Point:distance
        "distance", sol::overload (
            [] (PTF& self) { return self.getDistanceFromOrigin(); }, 
            [] (PTF& self, PTF& o) { return self.getDistanceFrom (o); }
        ),

        /// Distance from another point (squared).
        // @function Point:distanceSquared
        "distanceSquared", sol::overload (
            [] (PTF& self) { return self.getDistanceSquaredFromOrigin(); }, 
            [] (PTF& self, PTF& o) { return self.getDistanceSquaredFrom (o); }),

        /// Returns the angle from this point to another one.
        //
        // Taking this point to be the centre of a circle, and the other point being a position on
        // the circumference, the return value is the number of radians clockwise from the 12 o'clock
        // direction.
        // So 12 o'clock = 0, 3 o'clock = Pi/2, 6 o'clock = Pi, 9 o'clock = -Pi/2
        // @tparam el.Point other
        // @function Point:angleTo
        "angleTo", &PTF::getAngleToPoint,

        /// Get a rotated copy.
        // @function Point:rotated
        // @treturn el.Point
        "rotated", [](PTF& self, lua_Number angle) -> PTF {
            return self.rotatedAboutOrigin(angle);
        },

        /// Returns the dot product.
        // @function Point:dotProduct
        // @treturn number
        "dotProduct", &PTF::getDotProduct,

        /// Convert to integer values.
        // @function Point:toInt
        // @treturn int
        "toInt", &PTF::toInt
    );

    sol::stack::push (L, element::lua::removeAndClear (M, EL_TYPE_NAME_POINT));
    return 1;
}

// clang-format off
// Wrapper needed: Modern MSVC toolsets (14.5+) enforce stricter C++20 linkage rules.
// Lambdas with explicit return types inside extern "C" functions are flagged as errors.
// The wrapper isolates C linkage from C++ lambda code.
EL_PLUGIN_EXPORT
int luaopen_el_Point (lua_State* L)
{
    return register_el_Point(L);
}
