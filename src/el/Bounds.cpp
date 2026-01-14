// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

/// Bounding box
// The value type for this is a 32bit integer and Backed by a juce::Rectangle.
// API is identical to @{el.Rectangle}.
// @classmod el.Bounds
// @pragma nostrip

#include <element/element.hpp>
#include "rectangle.hpp"

#define EL_TYPE_NAME_BOUNDS "Bounds"

using namespace juce;

// clang-format off
EL_PLUGIN_EXPORT
int luaopen_el_Bounds (lua_State* L)
{
    using B = Rectangle<int>;

    auto M = element::lua::defineRectangle<int> (L, EL_TYPE_NAME_BOUNDS, sol::meta_method::to_string, [] (B& self) {
        return element::lua::to_string (self, EL_TYPE_NAME_BOUNDS);
    });

    sol::stack::push (L, M);
    return 1;
}
