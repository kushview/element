// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

/// A rectangle.
// The value type for this is a 64 bit float.
// @classmod el.Rectangle
// @pragma nostrip

#include <element/element.h>
#include "rectangle.hpp"
#define EL_TYPE_NAME_RECTANGLE "Rectangle"

namespace lua = element::lua;

// clang-format off
EL_PLUGIN_EXPORT
int luaopen_el_Rectangle (lua_State* L)
{
    using R = juce::Rectangle<lua_Number>;
    auto M = lua::defineRectangle<lua_Number> (L, EL_TYPE_NAME_RECTANGLE, 
        sol::meta_method::to_string, [] (R& self) {
            return lua::to_string (self, EL_TYPE_NAME_RECTANGLE);
        }
    );

    sol::stack::push (L, M);
    return 1;
}
