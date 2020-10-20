#include "JuceHeader.h"
#include "scripting/LuaBindings.h"
#include "scripting/ScriptDescription.h"
#include "scripting/ScriptManager.h"
#include "sol/sol.hpp"

namespace Element {

ScriptManager::ScriptManager()
{
    #if 0
    auto exepath = File::getSpecialLocation (File::invokedExecutableFile)
                        .getParentDirectory().getParentDirectory()
                        .getChildFile ("scripts")
                        .getFullPathName();

    setenv ("LUA_PATH", "", true);
    #endif
}

ScriptManager::~ScriptManager()
{

}

void ScriptManager::scanDirectory (File dir)
{
    sol::state lua;
    lua.open_libraries();
    Lua::openLibs (lua);
    
    DBG("[EL] scanning: " << dir.getFullPathName());
    for (DirectoryEntry entry : RangedDirectoryIterator (dir, false, "*.lua"))
    {
        sol::environment env (lua, sol::create, lua.globals());
        try {
            DBG("[EL] checking: " << entry.getFile().getFileName());
            lua.load_file (entry.getFile().getFullPathName().toStdString(), sol::load_mode::any);
            auto desc = ScriptDescription::parse (entry.getFile());
            DBG("[EL] script: " << desc.name);
        } catch (const std::exception& e) {
            DBG(e.what());
        }
    }
}

}
