/*
    This file is part of Element
    Copyright (C) 2019 Kushview, LLC.  All rights reserved.
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "scripting/Lua.h"
#include "JuceHeader.h"

namespace Element {

/** C++ RAII wrapper of a lua_State*
    This object should be used on the stack and can be passed to functions which
    take a lua_State* as a parameter.

    @warning Do not call lua_close() on this object. Because the dtor will do it 
    for you, you'll get a nasty crash from a double free of the C object.
*/
class LuaState final
{
public:
    LuaState();
    ~LuaState();

    bool isNull() const { return state == nullptr; }

    /** Pass as lua_State* as parameter. */
    operator lua_State*() const { return state; }

private:
    lua_State* state { nullptr };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LuaState)
};

}
