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

Array<ScriptDescription> ScriptManager::scanDirectory (File dir)
{
    sol::state lua;
    lua.open_libraries();
    Lua::openLibs (lua);

    Array<ScriptDescription> results;
    for (DirectoryEntry entry : RangedDirectoryIterator (dir, false, "*.lua"))
    {
        ScriptDescription desc;
        sol::environment env (lua, sol::create, lua.globals());
        try {
            auto result = lua.load_file (entry.getFile().getFullPathName().toStdString(), sol::load_mode::any);
            if (result.status() != sol::load_status::ok)
                continue;
            desc = ScriptDescription::parse (entry.getFile());
        } catch (const std::exception& e) {
            DBG(e.what());
            desc = {};
        }

        if (desc.isValid())
        {
            results.add (desc);
        }
    }
    
    return results;
}

}
