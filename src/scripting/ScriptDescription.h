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

#pragma once

#include "JuceHeader.h"
#include "sol/forward.hpp"

namespace Element {

struct ScriptDescription final
{
    String name;
    String type;
    String author;
    String description;
    String source;

    ScriptDescription() = default;
    ScriptDescription (const ScriptDescription& o) { operator= (o); }
    ~ScriptDescription() = default;
    
    ScriptDescription& operator= (const ScriptDescription& o)
    {
        this->name          = o.name;
        this->type          = o.type;
        this->author        = o.author;
        this->description   = o.description;
        this->source        = o.source;
        return *this;
    }

    static ScriptDescription read (lua_State*, const String& buffer);
    static ScriptDescription read (const String& buffer);
    static ScriptDescription read (File file);

    static ScriptDescription parse (const String& buffer);
    static ScriptDescription parse (File file);

    bool isValid() const { return name.isNotEmpty() && type.isNotEmpty(); }
};

}
