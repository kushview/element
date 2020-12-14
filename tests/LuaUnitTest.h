
#pragma once
#include "Tests.h"
#include "scripting/LuaBindings.h"
#include "sol/sol.hpp"

//=============================================================================
class LuaUnitTest : public Element::UnitTestBase
{
public:
    LuaUnitTest (const String& n, const String& c, const String& s)
        : UnitTestBase (n, c, s) {}

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
            [this](bool result, const char* msg) -> void {
                this->expect (result, String::fromUTF8 (msg));
            }
        );
    }

    void shutdown() override
    {
        lua.collect_garbage();

        shutdownWorld();
    }

protected:
    sol::state lua;
};
