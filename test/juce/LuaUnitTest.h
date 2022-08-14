
#pragma once
#include "Tests.h"
#include "scripting/bindings.hpp"
#include "sol/sol.hpp"

//=============================================================================
class LuaUnitTest : public Element::UnitTestBase
{
public:
    LuaUnitTest (const String& n, const String& c, const String& s)
        : UnitTestBase (n, c, s),
          state (new sol::state()),
          lua (*state) { }

    void initialise() override
    {
        Element::Lua::initializeState (lua);
        lua ["begintest"] = sol::overload (
            [this](const char* name) {
                beginTest (String::fromUTF8 (name));
            }
        );
        lua ["expect"] = sol::overload (
            [this](bool result) -> void {
                this->expect (result);
            } , 
            [this](bool result, sol::object obj) -> void {
                obj = lua["tostring"](obj);
                this->expect (result, obj.as<std::string>());
            }
        );

        resetPaths();
    }

    String getPath() const
    {
        String path (lua["package"]["path"].get_or<std::string>(""));
        DBG(path);
        return path;
    }

    /** Reset lua paths to defaults (in-tree locations for unit tests) */
    void resetPaths()
    {
        // by default
        auto package = lua["package"];
        const auto root = File::getCurrentWorkingDirectory().getFullPathName();
        String path; path << root << "/src/?.lua";
        package["path"] = path.toStdString();
        path.clear(); path << root << "/scripts/?.lua";
        package["spath"] = path.toStdString();
    }

    File getSnippetFile (const String& filename) const
    {
        String path = "test/snippets/"; path << filename;
        return File::getCurrentWorkingDirectory()
            .getChildFile (path);
    }

    std::string getSnippetPath (const String& filename) const
    {
        return getSnippetFile(filename).getFullPathName().replace (
            File::getCurrentWorkingDirectory().getFullPathName() + juce::File::getSeparatorString(),
            ""
        ).toStdString();
    }

    String readSnippet (const String& filename) const
    {
        return getSnippetFile(filename).loadFileAsString();
    }

    sol::call_status runSnippet (const String& filename)
    {
        try {
            auto res = lua.script_file (getSnippetPath (filename));
            if (! res.valid())
            {
                sol::error e = res;
                std::cerr << e.what();
            }
            return res.status();
        } catch (const std::exception&) {}
        return sol::call_status::handler;
    }

    void shutdown() override
    {
        lua.collect_garbage();
        state.reset();
        shutdownWorld();
    }

private:
    std::unique_ptr<sol::state> state;

protected:
    sol::state& lua;
};
