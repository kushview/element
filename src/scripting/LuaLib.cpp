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

#if EL_LUA_AMALGAMATED

#if defined(JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED)
#pragma error "Cannot include JUCE before LuaLib.cpp"
#pragma GCC error "Cannot include JUCE before LuaLib.cpp"
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4244) // convert possible data loss
#pragma warning(disable : 4297) // function assumed not to throw an exception but does
#pragma warning(disable : 4310) // cast truncates constant value
#pragma warning(disable : 4334) // result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)
#pragma warning(disable : 4701) // potentially uninitialized local variable
#pragma warning(disable : 4702) // unreachable code
#endif

extern "C" {
// #include "../../libs/element/lua/src/luac.c"
#include "../../libs/element/lua/src/lauxlib.c"
#include "../../libs/element/lua/src/lfunc.c"
#include "../../libs/element/lua/src/ltable.c"
#include "../../libs/element/lua/src/lzio.c"
#include "../../libs/element/lua/src/ldebug.c"
#include "../../libs/element/lua/src/ldblib.c"
#include "../../libs/element/lua/src/llex.c"
#include "../../libs/element/lua/src/lvm.c"
#include "../../libs/element/lua/src/lmathlib.c"
#include "../../libs/element/lua/src/lbaselib.c"
#include "../../libs/element/lua/src/ldump.c"
// #include "../../libs/element/lua/src/lua.c"
#include "../../libs/element/lua/src/lstate.c"
#include "../../libs/element/lua/src/lmem.c"
#include "../../libs/element/lua/src/lutf8lib.c"
#include "../../libs/element/lua/src/ltablib.c"
#include "../../libs/element/lua/src/ltm.c"
#include "../../libs/element/lua/src/lobject.c"
#include "../../libs/element/lua/src/liolib.c"
#include "../../libs/element/lua/src/lctype.c"
#include "../../libs/element/lua/src/lstring.c"
#include "../../libs/element/lua/src/loadlib.c"
#include "../../libs/element/lua/src/lcorolib.c"
#include "../../libs/element/lua/src/lstrlib.c"
#include "../../libs/element/lua/src/lparser.c"
#include "../../libs/element/lua/src/lcode.c"
#include "../../libs/element/lua/src/ldo.c"
#include "../../libs/element/lua/src/lapi.c"
#include "../../libs/element/lua/src/loslib.c"
#include "../../libs/element/lua/src/lopcodes.c"
#include "../../libs/element/lua/src/lgc.c"
#include "../../libs/element/lua/src/linit.c"
#include "../../libs/element/lua/src/lundump.c"
}

#endif
