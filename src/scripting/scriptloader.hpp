// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/script.hpp>
#include "sol/sol.hpp"

namespace element {

class ScriptLoader final : public juce::ReferenceCountedObject
{
public:
    ScriptLoader() = delete;
    explicit ScriptLoader (lua_State* L);
    ScriptLoader (lua_State* L, const juce::String& buffer);
    ScriptLoader (sol::state_view& view, const juce::String& buffer);
    ScriptLoader (lua_State* L, juce::File file);
    ScriptLoader (sol::state_view& view, juce::File file);

    ~ScriptLoader();

    bool isLoaded() const { return hasloaded; }
    bool isReady() const { return isLoaded() && ! hasError(); }
    bool load (const juce::String& buffer);
    bool load (juce::File file);

    sol::function caller() const
    {
        sol::function f = loaded;
        return f;
    }

    sol::object call (const sol::environment& env)
    {
        return execute (env);
    }

    sol::object operator() (const sol::environment& env)
    {
        return call (env);
    }

    template <typename Env, typename... Args>
    sol::object call (const Env& env, Args&&... args)
    {
        return execute (env, std::forward<Args> (args)...);
    }

    template <typename Env, typename... Args>
    sol::object operator() (const Env& env, Args&&... args)
    {
        return call (env, std::forward<Args> (args)...);
    }

    template <typename... Args>
    sol::object call (Args&&... args)
    {
        return execute (sol::environment(), std::forward<Args> (args)...);
    }

    template <typename... Args>
    sol::object operator() (Args&&... args)
    {
        return call (std::forward<Args> (args)...);
    }

    const auto& getInfo() const { return info; }
    juce::String getName() const { return info.name; }
    juce::String getType() const { return info.type; }
    juce::String getAuthor() const { return info.author; }
    juce::String getDescription() const { return info.description; }
    juce::String getSource() const { return info.code; }

    bool hasError() const { return error.isNotEmpty(); }
    juce::String getErrorMessage() const { return error; }

private:
    ScriptInfo info;
    lua_State* L = nullptr;
    bool ownedstate = false;
    bool hasloaded = false;
    sol::load_result loaded;
    juce::String error;

    template <typename... Args>
    sol::reference execute (const sol::environment& e, Args&&... args)
    {
        jassert (L != nullptr);
        sol::state_view view (L);
        sol::reference ref;
        try
        {
            ref = view.safe_script ("return nil");
            if (! isLoaded() || hasError())
                return ref;
            // env["testvalue"] = true;
            sol::function f = caller();
            if (e.valid())
                sol::set_environment (e, f);
            auto result = f (std::forward<Args> (args)...);
            switch (result.status())
            {
                case sol::call_status::file:
                    error = "File error";
                    break;
                case sol::call_status::gc:
                    error = "Garbage error";
                    break;
                case sol::call_status::handler:
                    error = "Handler error";
                    break;
                case sol::call_status::memory:
                    error = "Memory error";
                    break;
                case sol::call_status::ok:
                    ref = result;
                    error = "";
                    break;
                case sol::call_status::runtime:
                    error = "Runtime error";
                    break;
                case sol::call_status::syntax:
                    error = "Syntax error";
                    break;
                case sol::call_status::yielded:
                    error = "Yielded";
                    break;
                default:
                    error = "Unknown error";
                    break;
            }
        } catch (const std::exception& e)
        {
            error = juce::String ("exception: ") + e.what();
        }
        return ref;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScriptLoader);
};

} // namespace element
