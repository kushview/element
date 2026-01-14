// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

/// A rectangle.
// Easy to use Rectangle object backed.
// @classmod el.Rectangle

#pragma once

#include <element/juce/graphics.hpp>

#include "sol_helpers.hpp"

namespace element {
namespace lua {

// clang-format off
template <typename T, typename... Args>
inline static sol::table
    defineRectangle (lua_State* L, const char* name, Args&&... args)
{
    using R = juce::Rectangle<T>;
    sol::state_view lua (L);
    sol::table M = lua.create_table();
    M.new_usertype<R> (
        name, sol::no_constructor,
        /// Class Methods.
        // @section classmethods

        "new",
        sol::factories (
            /// Create a new empty rectangle.
            // @function Rectangle.new
            // @treturn el.Rectangle
            []() { return R(); },

            /// Create a new rectangle.
            // @function Rectangle.new
            // @param x
            // @param y
            // @param width
            // @param height
            // @treturn el.Rectangle
            [] (T x, T y, T w, T h) { return R (x, y, w, h); },

            /// Create a new rectangle.
            // @function Rectangle.new
            // @param width
            // @param height
            // @treturn el.Rectangle
            [] (T w, T h) { return R (w, h); },

            /// Create a new rectangle from two points.
            // @function Rectangle.new
            // @tparam el.Point p1
            // @tparam el.Point p2
            // @treturn el.Rectangle
            [] (juce::Point<T> p1, juce::Point<T> p2) { return R (p1, p2); }),

        /// Create a new rectangle from a set of coordinates.
        // @function Rectangle.fromcoords
        // @int x1 Left
        // @int y1 Top
        // @int x2 Right
        // @int y2 Bottom
        // @treturn el.Rectangle New rectangle
        "fromcoords",
        R::leftTopRightBottom,

        /// Attributes.
        // @section attributes

        /// @field Rectangle.x
        "x", sol::property (&R::getX, &R::setX),

        /// @field Rectangle.y
        "y", sol::property (&R::getY, &R::setY),

        /// @field Rectangle.width
        "width", sol::property (&R::getWidth, &R::setWidth),

        /// @field Rectangle.height
        "height",  sol::property (&R::getHeight, &R::setHeight),

        /// @field Rectangle.left
        "left", sol::property (&R::getX, &R::setLeft),

        /// @field Rectangle.right
        "right",  sol::property (&R::getRight, &R::setRight),

        /// @field Rectangle.top
        "top", sol::property (&R::getY, &R::setTop),

        /// @field Rectangle.bottom
        "bottom", sol::property (&R::getBottom, &R::setBottom),

        /// Methods.
        // @section methods

        /// Returns the center point of this rect.
        // @function Rectangle:center
        // @treturn el.Point
        "center", [] (R& self) {
            juce::Point<lua_Number> pt ((lua_Number) self.getCentreX(),
                                        (lua_Number) self.getCentreY());
            return pt;
        },

        /// @function Rectangle:centerX
        "centerX", &R::getCentreX,

        /// @function Rectangle:centerY
        "centerY", &R::getCentreY,

        /// Is empty.
        // @function Rectangle:empty
        // @return True if a 0,0,0,0 rectangle
        "empty", &R::isEmpty,

        /// Is finite.
        // @function Rectangle:isFinite
        // @return True if finite
        "isFinite",
        &R::isFinite,

        /// Translate the rectangle.
        // @function Rectangle:translate
        // @param dx
        // @param dy
        "translate",
        &R::translate,

        /// Returns a translated retctangle.
        // @function Rectangle:translated
        // @param dx
        // @param dy
        // @return A translated rectangle
        "translated",  &R::translated,

        /// Expand the rectangle in size.
        // @function Rectangle:expand
        // @param dx
        // @param dy
        "expand",  &R::expand,

        "expanded", sol::overload (
            /// Returns expanded rectangle.
            // @function Rectangle:expanded
            // @param dx
            // @param dy
            [] (R& self, T dx, T dy) { return self.expanded (dx, dy); },

            /// Returns expanded rectangle.
            // @function Rectangle:expanded
            // @param dxy Delta X and Y
            [] (R& self, T d) { return self.expanded (d); }),

        /// Reduce the rectangle in size.
        // @function Rectangle:reduce
        // @param dx
        // @param dy
        "reduce", &R::reduce,

        "reduced", sol::overload (
            /// Returns reduced rectangle.
            // @function Rectangle:reduced
            // @param dx
            // @param dy
            [] (R& self, T dx, T dy) { return self.reduced (dx, dy); },

            /// Returns reduced rectangle.
            // @function Rectangle:reduced
            // @param dxy Delta X and Y
            [] (R& self, T d) { return self.reduced (d); }),

        /// Slice top.
        // Remomve and return a portion of this rectangle.
        // @function Rectangle:sliceTop
        // @param amt Amount to remove
        "sliceTop",  &R::removeFromTop,

        /// Slice left.
        // Remomve and return a portion of this rectangle.
        // @function Rectangle:sliceLeft
        // @param amt Amount to remove
        "sliceLeft", &R::removeFromLeft,

        /// Slice right.
        // Remomve and return a portion of this rectangle.
        // @function Rectangle:sliceRight
        // @param amt Amount to remove
        "sliceRight",  &R::removeFromRight,

        /// Slice bottom.
        // Remomve and return a portion of this rectangle.
        // @function Rectangle:sliceBottom
        // @param amt Amount to remove
        "sliceBottom", &R::removeFromBottom,

        /// Convert to integer.
        // @function Rectangle:toInt
        // @treturn el.Bounds Converted rectangle
        "toInt", &R::toNearestInt,

        /// Convert to rounded integer.
        // @function Rectangle:toEdges
        // @treturn el.Bounds Converted rectangle
        "toEdges", &R::toNearestIntEdges,

        /// Convert to number.
        // @function Rectangle:toNumber
        // @return Converted rectangle
        "toNumber", &R::toDouble,
        std::forward<Args> (args)...
#if 0        
        "getAspectRatio", sol::overload (
            [](R& self) { return self.getAspectRatio(); },
            [](R& self, bool widthOverHeight) { return self.getAspectRatio (widthOverHeight); }
        ),

        "get_position",     &R::getPosition,
        "set_position", sol::overload (
            sol::resolve<void(Point<T>)> (&R::setPosition),
            sol::resolve<void(T, T)> (&R::setPosition)
        ),

        "getTopLeft",           &R::getTopLeft,
        "getTopRight",          &R::getTopRight,
        "getBottomLeft",        &R::getBottomLeft,
        "getBottomRight",       &R::getBottomRight,
        "getHorizontalRange",   &R::getHorizontalRange,
        "getVerticalRange",     &R::getVerticalRange,
        "setSize",              &R::setSize,
        "setBounds",            &R::setBounds,
        "setX",                 &R::setX,
        "setY",                 &R::setY,
        "setWidth",             &R::setWidth,
        "setHeight",            &R::setHeight,
        "setCentre", sol::overload (
            sol::resolve<void(T, T)> (&R::setCentre),
            sol::resolve<void(Point<T>)> (&R::setCentre)
        ),
        "setHorizontalRange",   &R::setHorizontalRange,
        "setVerticalRange",     &R::setVerticalRange,
        "withX",                &R::withX,
        "withY",                &R::withY,
        "withRightX",           &R::withRightX,
        "withBottomY",          &R::withBottomY,
        "withPosition", sol::overload (
            [](R& self, T x, T y)       { return self.withPosition (x, y); },
            [](R& self, Point<T> pt)    { return self.withPosition (pt); }
        ),
        "withZeroOrigin",       &R::withZeroOrigin,
        "withCentre",           &R::withCentre,

        "withWidth",            &R::withWidth,
        "withHeight",           &R::withHeight,
        "withSize",             &R::withSize,
        "withSizeKeepingCentre",&R::withSizeKeepingCentre,
        "setLeft",              &R::setLeft,
        "withLeft",             &R::withLeft,

        "setTop",               &R::setTop,
        "withTop",              &R::withTop,
        "setRight",             &R::setRight,
        "withRight",            &R::withRight,
        "setBottom",            &R::setBottom,
        "withBottom",           &R::withBottom,
        "withTrimmedLeft",      &R::withTrimmedLeft,
        "withTrimmedRight",     &R::withTrimmedRight,
        "withTrimmedTop",       &R::withTrimmedTop,
        "withTrimmedBottom",    &R::withTrimmedBottom,
        
        "getConstrainedPoint",  &R::getConstrainedPoint,

        "getRelativePoint",     [](R& self, T rx, T ry) { return self.getRelativePoint (rx, ry); },
        "proportionOfWidth",    [](R& self, T p)        { return self.proportionOfWidth (p); },
        "proportionOfHeight",   [](R& self, T p)        { return self.proportionOfHeight (p); },
        "getProportion",        [](R& self, R pr)       { return self.getProportion (pr); }
#endif
    );

    return element::lua::removeAndClear (M, name);
}
// clang-format on

} // namespace lua
} // namespace element
