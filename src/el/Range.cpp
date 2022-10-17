
/// A numeric value range.
// @classmod el.Range
// @pragma nostrip

#include <element/element.h>
#include "sol_helpers.hpp"
#define LKV_TYPE_NAME_RANGE "Range"

using namespace juce;
namespace lua = element::lua;

EL_PLUGIN_EXPORT
int luaopen_el_Range (lua_State* L)
{
    sol::state_view lua (L);
    using RT = Range<double>;
    auto M = lua.create_table();
    M.new_usertype<RT> (
        LKV_TYPE_NAME_RANGE, sol::no_constructor, "new", sol::factories (
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
            return lua::to_string (self, LKV_TYPE_NAME_RANGE);
        },

        /// Max value.
        // @class field
        // @name Range.start
        // @within Attributes
        "min",
        sol::property (&RT::getStart, &RT::setStart),

        /// Min value.
        // @class field
        // @name Range.end
        // @within Attributes
        "max",
        sol::property (&RT::getEnd, &RT::setEnd),

        /// Methods.
        // @section methods

        /// Returns true if the range has a length of zero.
        // @function Range:isempty
        // @treturn bool
        "isempty",
        &RT::isEmpty,

        /// Returns the length (max - min).
        // @function Range:length
        // @treturn number
        "length",
        &RT::getLength,

        /// Set the length.
        // @function Range:setlength
        // @number length New length to set.
        // @treturn number
        "setlength",
        &RT::setLength);

    sol::stack::push (L, element::lua::remove_and_clear (M, LKV_TYPE_NAME_RANGE));
    return 1;
}
