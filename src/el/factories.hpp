// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "element/element.h"
#include "sol_helpers.hpp"
#include <element/juce.hpp>

namespace element {
namespace lua {

template <typename T>
T** new_userdata (lua_State* L, const char* metatable)
{
    T** data = (T**) lua_newuserdata (L, sizeof (T**));
    if (strlen (metatable) > 0)
        luaL_setmetatable (L, metatable);
    *data = new T();
    return data;
}

template <typename T, typename... Args>
T** new_userdata (lua_State* L, const char* metatable, Args&&... args)
{
    T** data = (T**) lua_newuserdata (L, sizeof (T**));
    if (strlen (metatable) > 0)
        luaL_setmetatable (L, metatable);
    *data = new T (std::forward<Args> (args)...);
    return data;
}

} // namespace lua
} // namespace element
