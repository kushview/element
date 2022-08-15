/*
    This file is part of Element
    Copyright (C) 2014-2020  Kushview, LLC.  All rights reserved.

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

#include "sol/forward.hpp"

namespace element {

class Context;

namespace Lua {
    extern void initializeState (sol::state_view&);
    extern void initializeState (sol::state_view&, Context&);
    extern void setGlobals (sol::state_view&, Context&);
    extern void clearGlobals (sol::state_view&);
} // namespace Lua

} // namespace element
