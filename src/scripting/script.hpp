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

#include "scripting/scriptdescription.hpp"
#include "sol/sol.hpp"

namespace Element {

class Script final : public ReferenceCountedObject
{
public:
    Script() = delete;
    explicit Script (lua_State* L);
    Script (sol::state_view& view, const String& buffer);
    Script (sol::state_view& view, File file);
    ~Script();

    bool isLoaded() const { return hasloaded; }
    bool isReady() const { return isLoaded() && ! hasError(); }
    bool load (const String& buffer);
    bool load (File file);

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
    String getName() const { return info.name; }
    String getType() const { return info.type; }
    String getAuthor() const { return info.author; }
    String getDescription() const { return info.description; }
    String getSource() const { return info.source; }

    bool hasError() const { return error.isNotEmpty(); }
    String getErrorMessage() const { return error; }

private:
    ScriptDescription info;
    lua_State* L = nullptr;
    bool ownedstate = false;
    bool hasloaded = false;
    sol::load_result loaded;
    String error;

    template <typename... Args>
    sol::reference execute (const sol::environment& e, Args&&... args)
    {
        jassert (L != nullptr);
        sol::state_view view (L);
        sol::reference ref = view.safe_script ("return nil");
        if (! isLoaded() || hasError())
            return ref;
        try
        {
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
            error = String ("exception: ") + e.what();
        }
        return ref;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Script);
};

} // namespace Element
