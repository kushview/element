// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

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

ScriptLoader::ScriptLoader (lua_State* L, const String& buffer)
    : ScriptLoader (L)
{
    this->load (buffer);
}

ScriptLoader::ScriptLoader (sol::state_view& view, const String& buffer)
    : ScriptLoader (view.lua_state())
{
    this->load (buffer);
}

ScriptLoader::ScriptLoader (lua_State* L, File file)
    : ScriptLoader (L)
{
    this->load (file);
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
        if (loaded.status() != sol::load_status::ok)
        {
            sol::error err = loaded;
            error = err.what();
        }
    } catch (const std::exception& e)
    {
        error = String ("exception: ") + e.what();
    }

    hasloaded = error.isEmpty();
    return hasloaded;
}

} // namespace element
