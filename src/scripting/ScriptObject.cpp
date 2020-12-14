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

#include "ScriptDescription.h"
#include "scripting/ScriptObject.h"
#include "sol/sol.hpp"

namespace Element {

struct ScriptObject::Impl
{
    ScriptObject& owner;
    sol::reference exported;
    sol::load_result loaded;
    ScriptDescription info;
    String error = "";

    Impl (ScriptObject& o) : owner(o) {}

    ~Impl()
    {
        exported = sol::lua_nil;
    }

    bool isLoaded() const { return loaded.status() == sol::load_status::ok; }

    bool loadBuffer (sol::state_view& view, const String& buffer)
    {
        if (isLoaded())
            return true;

        info = ScriptDescription::parse (buffer);
        error = info.isValid() ? "" : "Invalid info";

        try {
            loaded = view.load_buffer (buffer.toRawUTF8(), (size_t) buffer.length());
            switch (loaded.status())
            {
                case sol::load_status::file:
                    error = "File error";
                    break;
                case sol::load_status::gc:
                    error = "Garbace error";
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
        } catch (const std::exception& e) {
            error = String("exception: ") + e.what();
        }
        return error.isEmpty();
    }

    template<typename ...Args>
    ScriptResult call (Args&& ...args)
    {
        sol::protected_function_result result;
        if (! isLoaded())
            return result;
        try {
            result = loaded.call (std::forward<Args> (args)...);
            switch (result.status()) {
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

            // if (error.isNotEmpty())
            //     return false;

            // sol::table descriptor;
            // if (result.get_type() == sol::type::table)
            //     descriptor = result;
            // if (! descriptor.valid())
            // {
            //     error = "Invalid descriptor";
            // }
            // else
            // {
            //     name        = descriptor["name"].get_or<std::string> ("");
            //     type        = descriptor["type"].get_or<std::string> ("");
            //     author      = descriptor["author"].get_or<std::string> ("");
            //     description = descriptor["description"].get_or<std::string> ("");
            //     exported    = descriptor["export"];
            //     if (! owner.validateExport (exported))
            //         error = "Invalid export";
            // }
        }
        catch (const std::exception& e) {
            error = String("exception: ") + e.what();
        }
        return result;
    }
};

ScriptObject::ScriptObject ()
{
    impl.reset (new Impl (*this));
}

ScriptObject::~ScriptObject()
{
    impl.reset();
}

bool ScriptObject::validateExport (const sol::reference& ref) { return ref.valid(); }

bool ScriptObject::load (sol::state_view& view, const String& buffer)
{
    if (impl->loadBuffer (view, buffer))
    {
        loaded();
        return true;
    }
    return false;
}

ScriptResult ScriptObject::call()
{
    return impl->call();
}

ScriptResult ScriptObject::loadAndCall (sol::state_view& view, const String& buffer)
{
    if (load (view, buffer))
        return impl->call();
    return {};
}

String ScriptObject::getName()            const { return impl->info.name; }
String ScriptObject::getType()            const { return impl->info.type; }
String ScriptObject::getAuthor()          const { return impl->info.author; }
String ScriptObject::getDescription()     const { return impl->info.description; }
String ScriptObject::getSource()          const { return impl->info.source; }
bool ScriptObject::hasError()             const { return impl->error.isNotEmpty(); }
String ScriptObject::getErrorMessage()    const { return impl->error; }

}
