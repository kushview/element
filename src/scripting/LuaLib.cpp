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

#if 1
#include "sol/sol.hpp"

#if defined(JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED)
 #pragma error "Cannot include JUCE before LuaLib.cpp"
 #pragma GCC error "Cannot include JUCE before LuaLib.cpp"
#endif

#ifdef _MSC_VER
 #pragma warning(disable: 4244) // convert possible data loss
 #pragma warning(disable: 4310) // cast truncates constant value
 #pragma warning(disable: 4334) // result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)
#endif

#include "../../libs/lua/src/lauxlib.c"
#include "../../libs/lua/src/liolib.c"
#include "../../libs/lua/src/lopcodes.c"
#include "../../libs/lua/src/lstate.c"
#include "../../libs/lua/src/lobject.c"
#include "../../libs/lua/src/lmathlib.c"
#include "../../libs/lua/src/loadlib.c"
#include "../../libs/lua/src/lvm.c"
#include "../../libs/lua/src/lfunc.c"
#include "../../libs/lua/src/lstrlib.c"
#include "../../libs/lua/src/linit.c"
#include "../../libs/lua/src/lstring.c"
#include "../../libs/lua/src/lundump.c"
#include "../../libs/lua/src/lctype.c"
#include "../../libs/lua/src/ltable.c"
#include "../../libs/lua/src/ldump.c"
#include "../../libs/lua/src/loslib.c"
#include "../../libs/lua/src/lgc.c"
#include "../../libs/lua/src/lzio.c"
#include "../../libs/lua/src/ldblib.c"
#include "../../libs/lua/src/lutf8lib.c"
#include "../../libs/lua/src/lmem.c"
#include "../../libs/lua/src/lcorolib.c"
#include "../../libs/lua/src/lcode.c"
#include "../../libs/lua/src/ltablib.c"
#include "../../libs/lua/src/lapi.c"
#include "../../libs/lua/src/lbaselib.c"
#include "../../libs/lua/src/ldebug.c"
#include "../../libs/lua/src/lparser.c"
#include "../../libs/lua/src/llex.c"
#include "../../libs/lua/src/ltm.c"
#include "../../libs/lua/src/ldo.c"

#endif
