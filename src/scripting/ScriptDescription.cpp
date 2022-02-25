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

#include "scripting/ScriptDescription.h"
#include "scripting/LuaBindings.h"
#include "sol/sol.hpp"

namespace Element {

static ScriptDescription parseScriptComments (const String& buffer)
{
    static const StringArray tags = { "@author", "@script", "@description", "@kind" };

    ScriptDescription desc;
    desc.type = "";
    const auto lines = StringArray::fromLines (buffer);
    int index = 0;
    bool inBlock = false;
    bool finished = false;
    for (index = 0; index < lines.size(); ++index)
    {
        const auto line = lines[index].trim();

        if (! inBlock)
            inBlock = line.startsWith ("--[[");

        if (inBlock || line.startsWith ("--"))
        {
            for (const auto& tag : tags)
            {
                if (line.contains (tag))
                {
                    const auto value = line.fromFirstOccurrenceOf (tag, false, false).trimStart().upToFirstOccurrenceOf ("--]]", false, false).trimEnd();

                    // DBG (tag.replace("@","") << " = " << value);

                    if (tag == "@kind" && desc.type.isEmpty())
                    {
                        desc.type = value.fromLastOccurrenceOf (".", false, false);
                    }
                    else if (tag == "@script" && desc.name.isEmpty())
                    {
                        desc.name = value;
                    }
                    else if (tag == "@author" && desc.author.isEmpty())
                    {
                        desc.author = value;
                    }
                    else if (tag == "@description" && desc.description.isEmpty())
                    {
                        desc.description = value;
                    }
                }
            }

            if (inBlock)
            {
                inBlock = ! line.contains ("--]]");
                finished = ! inBlock;
            }
        }
        else if (! inBlock && ! line.startsWith ("--"))
        {
            finished = true;
        }

        if (finished)
        {
            // DBG("finihed at line index: " << index);
            // DBG("LINE: " << lines[index]);
            break;
        }
    }

    return desc;
}

ScriptDescription ScriptDescription::read (lua_State* L, const String& buffer)
{
    sol::state_view view (L);
    ScriptDescription desc;
    try
    {
        sol::table script;
        auto result = view.script (buffer.toRawUTF8());
        if (result.get_type() == sol::type::table)
            script = result;

        if (script.valid())
        {
            desc.name = script["name"].get_or<std::string> ("");
            desc.type = script["type"].get_or<std::string> ("");
            desc.author = script["author"].get_or<std::string> ("");
            desc.description = script["description"].get_or<std::string> ("");
        }
    } catch (const std::exception&)
    {
        desc = {};
    }

    return desc;
}

ScriptDescription ScriptDescription::read (const String& buffer)
{
    ScriptDescription desc;
    sol::state lua;
    Element::Lua::initializeState (lua);
    return read (lua, buffer);
}

ScriptDescription ScriptDescription::read (File file)
{
    return read (file.loadFileAsString());
}

ScriptDescription ScriptDescription::parse (const String& buffer)
{
    auto desc = parseScriptComments (buffer);
    desc.source = "";
    return desc;
}

ScriptDescription ScriptDescription::parse (File file)
{
    ScriptDescription desc;

    if (file.existsAsFile())
    {
        desc = parseScriptComments (file.loadFileAsString());
        desc.source = URL (file).toString (false);
    }

    return desc;
}

} // namespace Element
