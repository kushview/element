// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <sol/sol.hpp>

namespace element {
namespace lua {

template <class DerivedType>
class Object : public DerivedType
{
public:
    Object() = default;
    ~Object() = default;

    template <typename... Args>
    static void addMethods (const sol::table& T, Args&&... args)
    {
        sol::table T_mt = T[sol::metatable_key];
        T_mt["__methods"].get_or_create<sol::table>().add (std::forward<Args> (args)...);
    }

    template <typename... Args>
    static void exportAttributes (const sol::table& T, Args&&... args)
    {
        sol::table T_mt = T[sol::metatable_key];
        T_mt["__props"].get_or_create<sol::table>().add (std::forward<Args> (args)...);
    }
};

template <typename T>
static T* object_userdata (const sol::table& proxy)
{
    if (! proxy.valid())
        return nullptr;
    auto mt = proxy[sol::metatable_key];
    T* result = nullptr;

    try
    {
        if (mt["__impl"].get_type() == sol::type::userdata)
        {
            sol::object ud = mt["__impl"];
            result = ud.as<T*>();
        }
    } catch (const std::exception&)
    {
    }

    return result;
}

} // namespace lua
} // namespace element
