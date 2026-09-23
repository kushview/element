// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sol/sol.hpp"
#include "scripting.hpp"
#include "scripting/scriptmanager.hpp"
#include "scripting/bindings.hpp"
#include <element/context.hpp>

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
        consoleEnv = sol::environment();
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
    element::lua::PackageLoaderMap packages;
    // Declared after `state` so it is released before the state is closed.
    sol::environment consoleEnv;
    juce::StringArray consoleHistory;

    /** global table key to state reference */
    static constexpr const char* refkey = "__state";

    static State& getref (sol::state_view& view)
    {
        return view.globals()[refkey];
    }

    /** Inserts the runtime package searcher after Lua's preload searcher.
        `Lua::initializeState` later inserts the internal-module searcher ahead
        of it, so both stay active. */
    static void init_packages (lua_State* L)
    {
        sol::state_view view (L);
        view.open_libraries (sol::lib::package);
        auto package = view["package"];

        const char* skey = LUA_VERSION_NUM < 502 ? "loaders" : "searchers";
        sol::table orig_searchers = package[skey];
        auto new_searchers = view.create_table();
        new_searchers.add (orig_searchers[1]); // first searcher is the preloader
        new_searchers.add (resolve_internal_package); // insert ours
        for (size_t i = 2; i <= orig_searchers.size(); ++i) // add everything after (file searchers)
            new_searchers.add (package[skey][i]); // ..
        package[skey] = new_searchers; // replace them
    }

    /** Searcher for packages registered at runtime via `addPackage`. */
    static int resolve_internal_package (lua_State* L)
    {
        sol::state_view view (L);
        auto& state = getref (view);

        const auto mid = sol::stack::get<std::string> (L);
        auto it = state.packages.find (mid);
        if (it != state.packages.end())
        {
            if (nullptr != it->second)
                sol::stack::push (L, it->second);
            else
                lua_pushfstring (L, "\n\tno cfunction: lua_CFunction not present: %s", mid.c_str());
        }
        else
        {
            lua_pushfstring (L, "\n\tno field packages['%s']", mid.c_str());
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
        state->consoleEnv = sol::environment();
        Lua::clearGlobals (state->state);
        state->collectGarbage();
        state.reset();
    }
    world = nullptr;
}

void ScriptingEngine::logError (const String& msg)
{
    std::clog << "[scripting] " << msg.toStdString() << std::endl;
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

//=============================================================================
sol::environment& ScriptingEngine::consoleEnvironment()
{
    auto& env = state->consoleEnv;
    if (! env.valid())
    {
        sol::state_view lua (state->state);
        env = sol::environment (lua, sol::create, lua.globals());
    }
    return env;
}

juce::StringArray& ScriptingEngine::consoleHistory()
{
    return state->consoleHistory;
}

juce::Result ScriptingEngine::execute (const juce::String& code,
                                       sol::environment env,
                                       const juce::String& chunkName)
{
    JUCE_ASSERT_MESSAGE_THREAD
    sol::state_view lua (state->state);
    const auto chunk = "=" + chunkName.toStdString();

    try
    {
        bool haveReturn = true;
        juce::String buffer;
        buffer << "return " << code << ";";
        auto loaded = lua.load_buffer (buffer.toRawUTF8(), buffer.getNumBytesAsUTF8(), chunk, sol::load_mode::text);

        if (! loaded.valid())
        {
            haveReturn = false;
            loaded = lua.load_buffer (code.toRawUTF8(), code.getNumBytesAsUTF8(), chunk, sol::load_mode::text);
        }

        if (! loaded.valid())
        {
            sol::error error = loaded;
            return juce::Result::fail (error.what());
        }

        sol::protected_function fn = loaded;
        if (env.valid())
            sol::set_environment (env, fn);

        sol::protected_function traceback = lua["debug"]["traceback"];
        if (traceback.valid())
            fn.set_error_handler (traceback);

        auto result = fn();
        if (! result.valid())
        {
            sol::error error = result;
            return juce::Result::fail (error.what());
        }

        if (haveReturn && result.return_count() > 0)
        {
            sol::protected_function print;
            if (env.valid())
                print = env["print"];
            else
                print = lua["print"];
            if (print.valid())
            {
                auto printed = print (result);
                if (! printed.valid())
                {
                    sol::error error = printed;
                    return juce::Result::fail (error.what());
                }
            }
        }
    } catch (const std::exception& e)
    {
        return juce::Result::fail (e.what());
    }

    return juce::Result::ok();
}

} // namespace element
