// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

/// A rectangle.
// The value type for this is a 32 bit float.
// @classmod el.Rectangle
// @pragma nostrip

#include <element/element.h>
#include "rectangle.hpp"
#define LKV_TYPE_NAME_RECTANGLE "Rectangle"

namespace lua = element::lua;

EL_PLUGIN_EXPORT
int luaopen_el_Rectangle (lua_State* L)
{
    using R = juce::Rectangle<float>;

    auto M = lua::new_rectangle<float> (L, LKV_TYPE_NAME_RECTANGLE, sol::meta_method::to_string, [] (R& self) {
        return lua::to_string (self, LKV_TYPE_NAME_RECTANGLE);
    });

    sol::stack::push (L, M);
    return 1;
}
