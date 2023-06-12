/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

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
#include "scripting.hpp"
#include "scripting/scriptmanager.hpp"
#include "scripting/bindings.hpp"
#include <element/context.hpp>

#ifndef EL_LUA_SPATH
#define EL_LUA_SPATH ""
#endif

namespace element {

//=============================================================================
class ScriptingEngine::State
{
public:
    State() = delete;
    State (ScriptingEngine& s)
        : owner (s)
    {
        state.open_libraries (sol::lib::base, sol::lib::string);
        auto& g = state.globals();
        g.set (refkey, std::ref (*this));
        init_packages (state);
    }

    ~State()
    {
        auto& g = state.globals();
        g.set (refkey, sol::lua_nil);
    }

    operator lua_State*() const noexcept
    {
        return state.lua_state();
    }

    void collectGarbage()
    {
        state.collect_garbage();
    }

private:
    friend class ScriptingEngine;
    ScriptingEngine& owner;
    sol::state state;
    element::lua::PackageLoaderMap builtins;
    element::lua::PackageLoaderMap packages;

    /** global table key to state reference */
    static constexpr const char* refkey = "__state";

    static State& getref (sol::state_view& view)
    {
        return view.globals()[refkey];
    }

    static void init_packages (lua_State* L)
    {
        sol::state_view view (L);
        view.open_libraries (sol::lib::package);
#if 0
        // doing this destroys preloading capabilities in lua.
        view.clear_package_loaders();
        view.add_package_loader (resolve_internal_package);
#else
        auto package = view["package"];

        const char* skey = LUA_VERSION_NUM < 502 ? "loaders" : "searchers";
        sol::table orig_searchers = package[skey];
        auto new_searchers = view.create_table();
        new_searchers.add (orig_searchers[1]); // first searcher is the preloader
        new_searchers.add (resolve_internal_package); // insert ours
        for (int i = 2; i <= orig_searchers.size(); ++i) // add everything after (file searchers)
            new_searchers.add (package[skey][i]); // ..
        package[skey] = new_searchers; // replace them
#endif
    }

    /** Custom lua searcher handler */
    static int resolve_internal_package (lua_State* L)
    {
        sol::state_view view (L);
        auto& state = getref (view);

        // if (state.builtins.empty())
        //     element::lua::fill_builtins (state.builtins);

        const auto mid = sol::stack::get<std::string> (L);
        auto it = state.builtins.find (mid);
        auto end = state.builtins.end();
        std::string msgkey = "builtins";
        if (it == end)
        {
            it = state.packages.find (mid);
            end = state.packages.end();
            msgkey = "packages";
        }
        if (it != end)
        {
            if (nullptr != it->second)
                sol::stack::push (L, it->second);
            else
                lua_pushfstring (L, "\n\tno cfunction: lua_CFunction not present: %s", mid.c_str());
        }
        else
        {
            lua_pushfstring (L, "\n\tno field %s['%s']", msgkey.c_str(), mid.c_str());
        }

        return 1;
    }
};

void ScriptingEngine::addPackage (const std::string& name, element::lua::CFunction loader)
{
    auto& pkgs = state->packages;
    if (pkgs.find (name) == pkgs.end())
    {
        pkgs.insert ({ name, loader });
        std::clog << "Scripting::add_package(): inserted " << name << std::endl;
    }
}

std::vector<std::string> ScriptingEngine::getPackageNames() const noexcept
{
    std::vector<std::string> v;
    for (const auto& pkg : state->builtins)
        v.push_back (pkg.first);
    for (const auto& pkg : state->packages)
        v.push_back (pkg.first);
    std::sort (v.begin(), v.end());
    return v;
}

lua_State* ScriptingEngine::getLuaState() const { return *state; }

//=============================================================================
class ScriptingEngine::Impl
{
public:
    Impl (ScriptingEngine& e)
        : owner (e)
    {
    }

    ~Impl() {}

    void scanDefaultLoctaion()
    {
        manager.scanDefaultLocation();
    }

    ScriptManager& getManager() { return manager; }

private:
    friend class ScriptingEngine;
    ScriptingEngine& owner;
    ScriptManager manager;
};

//=============================================================================
ScriptingEngine::ScriptingEngine()
{
    state = std::make_unique<State> (*this);
    impl.reset (new Impl (*this));
    state->collectGarbage();
}

ScriptingEngine::~ScriptingEngine()
{
    if (state != nullptr)
    {
        Lua::clearGlobals (state->state);
        state->collectGarbage();
        state.reset();
    }
    world = nullptr;
}

void ScriptingEngine::logError (const String& msg)
{
    std::clog << msg.toStdString() << std::endl;
}

void ScriptingEngine::initialize (Context& g)
{
    world = &g;
    Lua::initializeState (state->state, g);
}

ScriptManager& ScriptingEngine::getScriptManager()
{
    return impl->manager;
}

} // namespace element
