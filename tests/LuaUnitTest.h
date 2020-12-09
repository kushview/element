
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
        Element::Lua::initializeState (lua, getWorld());
    }

    void shutdown() override
    {
        lua.collect_garbage();
        shutdownWorld();
    }

protected:
    sol::state lua;
};
