#include "JuceHeader.h"
#include "scripting/LuaBindings.h"
#include "scripting/ScriptManager.h"
#include "sol/sol.hpp"

namespace Element {

ScriptManager::ScriptManager()
{
}

ScriptManager::~ScriptManager()
{

}

void ScriptManager::scanDirectory (File dir)
{
    sol::state lua;
    lua.open_libraries();
    Lua::openLibs (lua);
    
    for (DirectoryEntry entry : RangedDirectoryIterator (dir, false, "*.lua"))
    {
        sol::environment env (lua, sol::create, lua.globals());
        try {
            lua.script_file (entry.getFile().getFullPathName().toStdString(), sol::load_mode::any);
        } catch (const std::exception& e) {
            DBG(e.what());
        }
    }
}

}
