#pragma once

extern "C" {
#include "element/detail/lauxlib.h"
#include "element/detail/lua.h"
#include "element/detail/lualib.h"
}

#include <map>
#include <string>

namespace element {
namespace lua {

using CFunction = lua_CFunction;
using PackageLoaderMap = std::map<std::string, CFunction>;

} // namespace lua
} // namespace element
