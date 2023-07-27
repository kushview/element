// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "sol/sol.hpp"

namespace element {

class ScriptInstance
{
public:
    ScriptInstance() = default;
    virtual ~ScriptInstance() = default;

    void cleanup()
    {
        if (! object.valid())
            return;

        switch (object.get_type())
        {
            case sol::type::table: {
                auto tbl = object.as<sol::table>();
                if (sol::function f = tbl["cleanup"])
                    f (object);
                break;
            }
            default:
                break;
        }
    }

private:
    sol::object object;
};

} // namespace element
