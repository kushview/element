/// A rectangle.
// Easy to use Rectangle object backed by JUCE Rectangle.
// @classmod kv.Rectangle

#pragma once

#include "lua-kv.hpp"
#include LKV_JUCE_HEADER

namespace kv {
namespace lua {

template<typename T, typename ...Args>
inline static sol::table
new_rectangle (lua_State* L, const char* name, Args&& ...args) {
    using R = juce::Rectangle<T>;
    sol::state_view lua (L);
    sol::table M = lua.create_table();
    M.new_usertype<R> (name, sol::no_constructor,
        /// Class Methods.
        // @section classmethods

        "new", sol::factories (
            /// Create a new empty rectangle.
            // @function Rectangle.new
            // @treturn kv.Rectangle
            []() { return R(); },

            /// Create a new rectangle.
            // @function Rectangle.new
            // @param x
            // @param y
            // @param width
            // @param height
            // @treturn kv.Rectangle
            [](T x, T y, T w, T h) { return R (x, y, w, h); },

            /// Create a new rectangle.
            // @function Rectangle.new
            // @param width
            // @param height
            // @treturn kv.Rectangle
            [](T w, T h) { return R (w, h); },

            /// Create a new rectangle from two points.
            // @function Rectangle.new
            // @tparam kv.Point p1
            // @tparam kv.Point p2
            // @treturn kv.Rectangle
            [](juce::Point<T> p1, juce::Point<T> p2) { return R (p1, p2); }
        ),

        /// Create a new rectangle from a set of coordinates. 
        // @function Rectangle.fromcoords
        // @int x1 Left
        // @int y1 Top
        // @int x2 Right
        // @int y2 Bottom
        // @treturn kv.Rectangle New rectangle
        "fromcoords",      R::leftTopRightBottom,

        /// Attributes.
        // @section attributes

        /// @field Rectangle.x
        "x",                sol::property (&R::getX, &R::setX),

        /// @field Rectangle.y
        "y",                sol::property (&R::getY, &R::setY),

        /// @field Rectangle.width
        "width",            sol::property (&R::getWidth, &R::setWidth),

        /// @field Rectangle.height
        "height",           sol::property (&R::getHeight, &R::setHeight),

        /// @field Rectangle.left
        "left",             sol::property (&R::getX, &R::setLeft),

        /// @field Rectangle.right
        "right",            sol::property (&R::getRight, &R::setRight),

        /// @field Rectangle.top
        "top",              sol::property (&R::getY, &R::setTop),

        /// @field Rectangle.bottom
        "bottom",           sol::property (&R::getBottom, &R::setBottom),

        /// Methods.
        // @section methods

        /// Returns the center point of this rect. 
        // @function Rectangle:center
        // @treturn kv.Point
        "center",           [](R& self) {
            juce::Point<lua_Number> pt ((lua_Number)self.getCentreX(), 
                                        (lua_Number)self.getCentreY());
            return pt;
        },

        /// @function Rectangle:centerx
        "centerx",          &R::getCentreX,

        /// @function Rectangle:centery
        "centery",          &R::getCentreY,

        /// Is empty.
        // @function Rectangle:isempty
        // @return True if a 0,0,0,0 rectangle
        "isempty",         &R::isEmpty,

        /// Is finite.
        // @function Rectangle:isfinite
        // @return True if finite
        "isfinite",        &R::isFinite,

        /// Translate the rectangle.
        // @function Rectangle:translate
        // @param dx
        // @param dy
        "translate",        &R::translate,

        /// Returns a translated retctangle.
        // @function Rectangle:translated
        // @param dx
        // @param dy
        // @return A translated rectangle
        "translated",       &R::translated,

        /// Expand the rectangle in size.
        // @function Rectangle:expand
        // @param dx
        // @param dy
        "expand",           &R::expand,
        
        
        "expanded", sol::overload (
            /// Returns expanded rectangle.
            // @function Rectangle:expanded
            // @param dx
            // @param dy
            [](R& self, T dx, T dy) { return self.expanded (dx, dy); },

            /// Returns expanded rectangle.
            // @function Rectangle:expanded
            // @param dxy Delta X and Y
            [](R& self, T d)        { return self.expanded (d); }
        ),
        
        /// Reduce the rectangle in size.
        // @function Rectangle:reduce
        // @param dx
        // @param dy
        "reduce",               &R::reduce,

        "reduced", sol::overload (
            /// Returns reduced rectangle.
            // @function Rectangle:reduced
            // @param dx
            // @param dy
            [](R& self, T dx, T dy) { return self.reduced (dx, dy); },

            /// Returns reduced rectangle.
            // @function Rectangle:reduced
            // @param dxy Delta X and Y
            [](R& self, T d)        { return self.reduced (d); }
        ),

        /// Slice top.
        // Remomve and return a portion of this rectangle.
        // @function Rectangle:slicetop
        // @param amt Amount to remove
        "slicetop",        &R::removeFromTop,

        /// Slice left.
        // Remomve and return a portion of this rectangle.
        // @function Rectangle:sliceleft
        // @param amt Amount to remove
        "sliceleft",       &R::removeFromLeft,

        /// Slice right.
        // Remomve and return a portion of this rectangle.
        // @function Rectangle:sliceright
        // @param amt Amount to remove
        "sliceright",      &R::removeFromRight,

        /// Slice bottom.
        // Remomve and return a portion of this rectangle.
        // @function Rectangle:slicebottom
        // @param amt Amount to remove
        "slicebottom",     &R::removeFromBottom,

        /// Convert to integer.
        // @function Rectangle:tointeger
        // @return Converted rectangle
        "tointeger",        &R::toNearestInt,

        /// Convert to rounded integer.
        // @function Rectangle:toedges
        // @return Converted rectangle
        "toedges",          &R::toNearestIntEdges,

        /// Convert to number.
        // @function Rectangle:tointeger
        // @return Converted rectangle
        "tonumber",         &R::toDouble,
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

    return kv::lua::remove_and_clear (M, name);
}

}}
