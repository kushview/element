// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

/// A numeric value range.
// @classmod el.Range
// @pragma nostrip

#include <element/element.h>
#include "sol_helpers.hpp"
#define EL_TYPE_NAME_RANGE "Range"

using namespace juce;
namespace lua = element::lua;

// clang-format off
EL_PLUGIN_EXPORT
int luaopen_el_Range (lua_State* L)
{
    sol::state_view lua (L);
    using RT = Range<lua_Number>;
    auto M = lua.create_table();
    M.new_usertype<RT> (
        EL_TYPE_NAME_RANGE, sol::no_constructor, "new", sol::factories (
        /// Create an empty range.
        // @function Range.new
        // @treturn kv.Range
        []() { return RT(); },

        /// Create a new range.
        // @function Range.new
        // @number start Start value
        // @number end End value
        // @treturn kv.Range
        [] (lua_Number x, lua_Number y) { return RT (x, y); }),

        sol::meta_method::to_string,
        [] (RT& self) {
            return lua::to_string (self, EL_TYPE_NAME_RANGE);
        },

        /// Minimum value.
        //
        // Set to change the start position of the range, leaving the end position unchanged.
        // If the new start position is higher than the current end of the range, the end point
        // will be pushed along to equal it, leaving an empty range at the new position.
        // @class field
        // @name Range.min
        // @within Attributes
        "min", sol::property (&RT::getStart, &RT::setStart),

        /// Maximum value.
        //
        // Changes the end position of the range, leaving the start unchanged.
        // If the new end position is below the current start of the range, the 
        // start point will be pushed back to equal the new end point.
        // @class field
        // @name Range.max
        // @within Attributes
        "max", sol::property (&RT::getEnd, &RT::setEnd),

        /// Methods.
        // @section methods

        /// Returns true if the range has a length of zero.
        // @function Range:empty
        // @treturn bool
        "empty", &RT::isEmpty,

        /// Returns the length (max - min).
        // @function Range:length
        // @treturn number
        "length",  &RT::getLength,

        /// Set the length.
        // @function Range:setLength
        // @number length New length to set.
        // @treturn number
        "setLength", &RT::setLength);

    sol::stack::push (L, element::lua::removeAndClear (M, EL_TYPE_NAME_RANGE));
    return 1;
}
