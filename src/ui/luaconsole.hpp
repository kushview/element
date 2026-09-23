// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce/core.hpp>

#include "ui/console.hpp"
#include "scripting.hpp"

namespace element {

/** Interactive console that evaluates Lua in the engine's persistent
    console environment.
*/
class LuaConsole : public Console,
                   private juce::Timer
{
public:
    LuaConsole();
    virtual ~LuaConsole();

    /** Binds the console to a scripting engine.

        Installs the console's `print`, `clear` and `os.exit` into the engine's
        console environment and runs the console prelude the first time the
        environment is used. Command history is restored from the engine.
    */
    void initialize (ScriptingEngine& engine);

    void textEntered (const String&) override;

private:
    ScriptingEngine* engine = nullptr;
    sol::environment env;
    juce::CriticalSection printLock;
    StringArray printMessages;

    void installEnvironment();
    void runPrelude();
    void addResultText (const juce::Result&);

    friend class juce::Timer;
    void timerCallback() override;
};

} // namespace element
