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
    juce::Result execute (const String& code);

    std::vector<std::string> getPackageNames() const noexcept;
    void addPackage (const std::string& name, lua::CFunction loader);

    void logError (const String&);

private:
    friend Context;
    class Impl;
    std::unique_ptr<Impl> impl;
    Context* world = nullptr;
    lua_State* L = nullptr;
    class State;
    std::unique_ptr<State> state;

    ScriptingEngine (const ScriptingEngine&) = delete;
    void initialize (Context&);
};

using Scripting = ScriptingEngine;

} // namespace element
