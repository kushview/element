#pragma once

struct lua_State;

namespace Element {

class LuaState
{
public:
    LuaState();
    ~LuaState();

private:
    lua_State* state { nullptr };
};

}
