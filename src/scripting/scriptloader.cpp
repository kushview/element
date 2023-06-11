/*
    This file is part of Element
    Copyright (C) 2020  Kushview, LLC.  All rights reserved.

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

#include "sol/sol.hpp"
#include "scripting/bindings.hpp"
#include <element/script.hpp>
#include "scripting/scriptloader.hpp"

namespace element {
using namespace juce;

ScriptLoader::ScriptLoader (lua_State* state)
{
    ownedstate = state == nullptr;
    L = ownedstate ? luaL_newstate() : state;
    sol::state_view view (L);

    if (ownedstate)
    {
        Lua::initializeState (view);
    }
}

ScriptLoader::ScriptLoader (sol::state_view& view, const String& buffer)
    : ScriptLoader (view.lua_state())
{
    this->load (buffer);
}

ScriptLoader::ScriptLoader (sol::state_view& view, File file)
    : ScriptLoader (view.lua_state())
{
    this->load (file);
}

ScriptLoader::~ScriptLoader()
{
    if (ownedstate)
    {
        {
            sol::state_view view (L);
            view.collect_garbage();
            loaded = {};
        }
        lua_close (L);
    }

    L = nullptr;
}

bool ScriptLoader::load (File file)
{
    bool res = load (file.loadFileAsString());
    info.code = URL (file).toString (false);
    return res;
}

bool ScriptLoader::load (const String& buffer)
{
    jassert (L != nullptr);
    if (L == nullptr)
        return false;

    sol::state_view view (L);
    info = ScriptInfo::parse (buffer);
    std::string chunk = info.name.isNotEmpty() ? info.name.toStdString() : "script=";
    error = "";

    try
    {
        loaded = view.load_buffer (buffer.toRawUTF8(), (size_t) buffer.length(), chunk);
        switch (loaded.status())
        {
            case sol::load_status::file:
                error = "File error";
                break;
            case sol::load_status::gc:
                error = "Garbage error";
                break;
            case sol::load_status::memory:
                error = "Memory error";
                break;
            case sol::load_status::syntax:
                error = "Syntax error";
                break;
            case sol::load_status::ok:
                error = "";
                break;
            default:
                error = "Unknown error";
                break;
        }
    } catch (const std::exception& e)
    {
        error = String ("exception: ") + e.what();
    }

    hasloaded = error.isEmpty();
    return hasloaded;
}

} // namespace element
