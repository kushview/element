
#pragma once

#include <element/juce/core.hpp>

#include "scripting/bindings.hpp"
#include "sol/sol.hpp"

//=============================================================================
class LuaFixture {
public:
    using juce::File;
    using juce::String;

    LuaFixture()
    {
        element::Lua::initializeState (state);
        lua["BOOST_REQUIRE"] = sol::overload (
            [this] (bool result) -> void {
                BOOST_REQUIRE (result);
            })
            resetPaths();
    }

    juce::String getPath() const
    {
        String path (lua["package"]["path"].get_or<std::string> (""));
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
        try {
            auto res = lua.script_file (getSnippetPath (filename));
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
