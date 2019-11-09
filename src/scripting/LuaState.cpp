
#include <lua.hpp>
#include "scripting/LuaState.h"

namespace Element {

LuaState::LuaState()
{
    state = luaL_newstate();
    luaL_openlibs (state);
}

LuaState::~LuaState()
{
    lua_close (state);
    state = nullptr;
}

}
