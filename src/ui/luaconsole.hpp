// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce/core.hpp>

#include "ui/console.hpp"
#include "scripting.hpp"

namespace element {

class LuaConsole : public Console,
                   private juce::Timer
{
public:
    LuaConsole();
    virtual ~LuaConsole();

    void textEntered (const String&) override;
    void setEnvironment (const sol::environment& e);

private:
    using LuaResult = sol::protected_function_result;
    sol::environment env;
    String lastError;
    StringArray printMessages;
    LuaResult errorHandler (lua_State* L, LuaResult pfr);

    friend class juce::Timer;
    void timerCallback() override;
};

} // namespace element
