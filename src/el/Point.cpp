// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

/// A pair of x,y coordinates.
// @classmod el.Point
// @pragma nostrip

#include <element/element.h>
#include <element/juce/graphics.hpp>

#include "sol_helpers.hpp"
#define LKV_TYPE_NAME_POINT "Point"

using namespace juce;

EL_PLUGIN_EXPORT
int luaopen_el_Point (lua_State* L)
{
    sol::state_view lua (L);
    using PTF = Point<lua_Number>;
    auto M = lua.create_table();
    M.new_usertype<PTF> (
        LKV_TYPE_NAME_POINT, sol::no_constructor, "new", sol::factories (
                                                             /// Create a new point with x and y = 0.
                                                             // @function Point.new
                                                             // @treturn el.Point
                                                             []() { return PTF(); },

                                                             /// Create a new point.
                                                             // @function Point.new
                                                             // @number x X coordinate
                                                             // @number y Y coordinate
                                                             // @treturn el.Point
                                                             [] (lua_Number x, lua_Number y) { return PTF (x, y); }),
        sol::meta_method::to_string,
        [] (PTF& self) {
            return self.toString().toStdString();
        },

        /// X coord.
        // @class field
        // @name Point.x
        // @within Attributes
        "x",
        sol::property (&PTF::getX, &PTF::setX),

        /// Y coord.
        // @class field
        // @name Point.x
        // @within Attributes
        "y",
        sol::property (&PTF::getY, &PTF::setY),

        /// Methods.
        // @section methods

        /// True if is the origin point
        // @function Point:isorigin
        // @within Methods
        "isorigin",
        &PTF::isOrigin,

        /// True if is finite
        // @function Point:isfinite
        // @within Methods
        "isfinite",
        &PTF::isFinite,

        /// Returns a point with the given x coordinate.
        // @param x New x coord
        // @function Point:withx
        // @treturn el.Point New point object
        "withx",
        &PTF::withX,

        /// Returns a point with the given y coordinate.
        // @param y New y coord
        // @function Point:withy
        // @treturn el.Point New point object
        "withy",
        &PTF::withY,

        /// Set x and y at the same time.
        // @number x New x coordinate
        // @number y New y coordinate
        // @function Point:setxy
        "setxy",
        &PTF::setXY,

        /// Adds a pair of coordinates to this value.
        // @number x X to add
        // @number y Y to add
        // @function Point:addxy
        "addxy",
        &PTF::addXY,

        /// Move the point by delta x and y.
        // @function Point:translated
        // @number deltax
        // @number deltay
        "translated",
        &PTF::translated,

        /// Distance to another point.
        // @function Point:distance
        "distance",
        sol::overload ([] (PTF& self) { return self.getDistanceFromOrigin(); }, [] (PTF& self, PTF& o) { return self.getDistanceFrom (o); }),

        /// Distance from another point (squared).
        // @function Point:distancesquared
        "distancesquared",
        sol::overload ([] (PTF& self) { return self.getDistanceSquaredFromOrigin(); }, [] (PTF& self, PTF& o) { return self.getDistanceSquaredFrom (o); }),

        /// Angle to another point.
        // @function Point:angleto
        "angleto",
        &PTF::getAngleToPoint,

        /// Get a rotated copy.
        // @function Point:rotated
        "rotated",
        &PTF::rotatedAboutOrigin,

        /// Returns the dot product.
        // @function Point:dotproduct
        "dotproduct",
        &PTF::getDotProduct,

        /// Convert to integer values.
        // @function Point:toint
        "toint",
        &PTF::toInt);

    sol::stack::push (L, element::lua::removeAndClear (M, LKV_TYPE_NAME_POINT));
    return 1;
}
