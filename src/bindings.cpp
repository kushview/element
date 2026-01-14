// Copyright 2019-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <map>

#include "element/element.hpp"
#include "element/lua.hpp"
#include <sol/sol.hpp>

#define EL_USE_LIBRARY_BUILT_INS 0

#if EL_USE_LIBRARY_BUILT_INS
extern "C" {
extern int luaopen_el_audio (lua_State*);
extern int luaopen_el_midi (lua_State*);
extern int luaopen_el_bytes (lua_State*);
extern int luaopen_el_round (lua_State*);
}
#endif

namespace element {
namespace lua {

void fill_builtins (PackageLoaderMap& pkgs)
{
#if EL_USE_LIBRARY_BUILT_INS
    if (pkgs.find ("el.audio") != pkgs.end())
        return;
    pkgs.insert (
        { { "el.audio", luaopen_el_audio },
          { "el.bytes", luaopen_el_bytes },
          { "el.midi", luaopen_el_midi },
          { "el.round", luaopen_el_round } });
#else
    element::ignore (pkgs);
#endif
}

} // namespace lua
} // namespace element
