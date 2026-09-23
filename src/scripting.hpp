// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce.hpp>

#include "element/lua.hpp"
#include "sol/sol.hpp"

namespace element {

class Context;
class ScriptManager;

class ScriptingEngine
{
public:
    ScriptingEngine();
    ~ScriptingEngine();

    ScriptManager& getScriptManager();

    //==========================================================================
    lua_State* getLuaState() const;

    //==========================================================================
    /** Compiles and runs a chunk of Lua in the given environment.

        The code is first compiled as an expression (`return <code>;`). When
        that succeeds and the chunk yields values, they are passed to the
        environment's `print` function, so a console can echo results. When it
        does not compile as an expression the code is run as a plain chunk.

        Runtime errors are caught with a `debug.traceback` handler and never
        propagate. Must be called on the message thread.

        @param code       The Lua source to run.
        @param env        The environment to run in. An invalid environment
                          falls back to the global table.
        @param chunkName  Name used in error messages and tracebacks.
        @return ok on success, otherwise the error message (with traceback
                for runtime errors).
    */
    juce::Result execute (const juce::String& code,
                          sol::environment env = {},
                          const juce::String& chunkName = "console");

    /** Returns the persistent console environment.

        Created on first use with a fallback to the globals table, so reads
        see everything in `_G` while writes stay local to the console. It
        lives as long as the engine, so console state survives the console
        view being closed and reopened.
    */
    sol::environment& consoleEnvironment();

    /** Returns the console's command history (persists with the engine). */
    juce::StringArray& consoleHistory();

    std::vector<std::string> getPackageNames() const noexcept;
    void addPackage (const std::string& name, lua::CFunction loader);

    void logError (const String&);

private:
    friend Context;
    class Impl;
    std::unique_ptr<Impl> impl;
    Context* world = nullptr;
    class State;
    std::unique_ptr<State> state;

    ScriptingEngine (const ScriptingEngine&) = delete;
    void initialize (Context&);
};

using Scripting = ScriptingEngine;

} // namespace element
