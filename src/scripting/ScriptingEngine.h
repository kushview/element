/*
    This file is part of Element
    Copyright (C) 2019-2020  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

#include "JuceHeader.h"
#include "sol/sol.hpp"

namespace Element {

class Globals;
class ScriptManager;

class ScriptingEngine
{
public:
    ScriptingEngine();
    ~ScriptingEngine();

    ScriptManager& getScriptManager();

    //==========================================================================
    lua_State* getLuaState() const { return lua.lua_state(); }
    sol::state& getState() { return lua; }
    const sol::state& getState() const { return lua; }

    //==========================================================================
    Result execute (const String& code);

private:
    friend Globals;
    class Impl;
    std::unique_ptr<Impl> impl;
    Globals* world = nullptr;
    sol::state lua;
    void initialize (Globals&);
};

} // namespace Element
