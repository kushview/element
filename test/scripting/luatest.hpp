// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <string>

#include <element/juce/core.hpp>

#include "scripting/bindings.hpp"
#include "sol/sol.hpp"
#include "testutil.hpp"

//=============================================================================
class LuaFixture {
public:
    using File = juce::File;
    using String = juce::String;

    LuaFixture()
    {
        element::Lua::initializeState (state);
        state["BOOST_REQUIRE"] = sol::overload (
            [] (bool result) -> void {
                BOOST_REQUIRE (result);
            });
        state["expect"] = state["BOOST_REQUIRE"];
        resetPaths();
    }

    lua_State* luaState() const { return state.lua_state(); }

    juce::String getPath() const
    {
        String path (state["package"]["path"].get_or<std::string> (""));
        DBG (path);
        return path;
    }

    /** Reset lua paths to defaults (in-tree locations for unit tests) */
    void resetPaths()
    {
        // by default
        auto package = state["package"];
        const auto root = File::getCurrentWorkingDirectory().getFullPathName();
        String path;
        path << root << "/src/?.lua";
        package["path"] = path.toStdString();
        path.clear();
        path << root << "/scripts/?.lua";
        package["spath"] = path.toStdString();
    }

    File getSnippetFile (const String& filename) const
    {
        return File (EL_TEST_SOURCE_ROOT)
            .getChildFile ("test/snippets")
            .getChildFile (filename);
    }

    String readSnippet (const String& filename) const
    {
        return getSnippetFile (filename).loadFileAsString();
    }

    sol::call_status runSnippet (const String& filename)
    {
        auto file = getSnippetFile (filename);
        auto path = file.getFullPathName().toStdString();
        try {
            auto res = state.script_file (path);
            if (! res.valid()) {
                sol::error e = res;
                std::cerr << e.what();
            }
            return res.status();
        } catch (const std::exception&) {
        }
        return sol::call_status::handler;
    }

private:
    sol::state state;
};
