#pragma once

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

#include <map>
#include <string>

namespace Element {
namespace lua {

using CFunction = lua_CFunction;
using PackageLoaderMap = std::map<std::string, CFunction>;

} // namespace lua
} // namespace Element
