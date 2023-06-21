// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

#include <map>
#include <string>

namespace element {
namespace lua {

using CFunction = lua_CFunction;
using PackageLoaderMap = std::map<std::string, CFunction>;

} // namespace lua
} // namespace element
