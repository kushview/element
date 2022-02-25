/*
    This file is part of Element
    Copyright (C) 2014-2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <memory>

#include "controllers/AppController.h"
#include "controllers/GuiController.h"
#include "engine/AudioEngine.h"
#include "engine/MidiPipe.h"
#include "gui/SystemTray.h"
#include "scripting/ScriptManager.h"
#include "session/CommandManager.h"
#include "session/MediaManager.h"
#include "session/Node.h"
#include "session/PluginManager.h"
#include "session/Presets.h"
#include "session/Session.h"
#include "DataPath.h"
#include "Settings.h"
#include "sol/sol.hpp"
#include "lua-kv.hpp"

#ifndef EL_LOCAL_LUA_PATHS
#define EL_LOCAL_LUA_PATHS 0
#endif

namespace sol {
/** Support juce::ReferenceCountedObjectPtr */
template <typename T>
struct unique_usertype_traits<ReferenceCountedObjectPtr<T>>
{
    typedef T type;
    typedef ReferenceCountedObjectPtr<T> actual_type;
    static const bool value = true;
    static bool is_null (const actual_type& ptr) { return ptr == nullptr; }
    static type* get (const actual_type& ptr) { return ptr.get(); }
};
} // namespace sol

#include "../../element/lua/el/CommandManager.cpp"
#include "../../element/lua/el/Globals.cpp"
#include "../../element/lua/el/Node.cpp"
#include "../../element/lua/el/Session.cpp"

extern "C" {
extern int luaopen_kv_audio (lua_State* L);
extern int luaopen_kv_bytes (lua_State*);
extern int luaopen_kv_midi (lua_State*);
extern int luaopen_kv_round (lua_State*);
}

extern int luaopen_kv_AudioBuffer32 (lua_State*);
extern int luaopen_kv_AudioBuffer64 (lua_State*);
extern int luaopen_kv_Bounds (lua_State*);
extern int luaopen_kv_TextButton (lua_State*);
extern int luaopen_kv_Widget (lua_State*);
extern int luaopen_kv_Desktop (lua_State*);
extern int luaopen_kv_DocumentWindow (lua_State*);
extern int luaopen_kv_File (lua_State*);
extern int luaopen_kv_Graphics (lua_State*);
extern int luaopen_kv_MidiBuffer (lua_State*);
extern int luaopen_kv_MidiMessage (lua_State*);
extern int luaopen_kv_MouseEvent (lua_State*);
extern int luaopen_kv_Point (lua_State*);
extern int luaopen_kv_Range (lua_State*);
extern int luaopen_kv_Rectangle (lua_State*);
extern int luaopen_kv_Slider (lua_State*);
extern int luaopen_el_MidiPipe (lua_State*);

using namespace sol;

namespace Element {
namespace Lua {

    //==============================================================================
    static File getAppImageLuaPath()
    {
#if defined(EL_APPIMAGE)
        return File::getSpecialLocation (File::currentExecutableFile)
            .getParentDirectory() // bin
            .getParentDirectory() // usr
            .getChildFile ("share/element/modules");
#endif
        jassertfalse;
        return File();
    }

    static File getTopDirectory()
    {
        File topdir = File::getSpecialLocation (File::currentExecutableFile).getParentDirectory();
        while (topdir.exists() && topdir.isDirectory())
        {
            if (topdir.getChildFile ("tools/waf/element.py").existsAsFile())
                return topdir;
            topdir = topdir.getParentDirectory();
        }
        return File();
    }

    //==============================================================================
    static String getLocalScriptsDir()
    {
        File dir;

#if JUCE_DEBUG && EL_LOCAL_LUA_PATHS
        File topdir = getTopDirectory();
        if (topdir.exists() && topdir.isDirectory())
            dir = topdir.getChildFile ("scripts");
#endif

        return dir.exists() ? dir.getFullPathName() : String();
    }

    static String getHomeScriptsDir()
    {
        return File::getSpecialLocation (File::userHomeDirectory)
            .getChildFile (".local/share/element/scripts")
            .getFullPathName();
    }

    static String getApplicationScriptsDir()
    {
        return ScriptManager::getApplicationScriptsDir().getFullPathName();
    }

    static String getUserScriptsDir()
    {
        return ScriptManager::getUserScriptsDir().getFullPathName();
    }

    static String getSystemScriptsDir()
    {
        return ScriptManager::getSystemScriptsDir().getFullPathName();
    }

    //==============================================================================
    static String getHomeLuaDir()
    {
        return File::getSpecialLocation (File::userHomeDirectory)
            .getChildFile (".local/share/element/modules")
            .getFullPathName();
    }

    static StringArray getLocalLuaDirs()
    {
        StringArray dirs;
#if JUCE_DEBUG && EL_LOCAL_LUA_PATHS
        auto topdir = getTopDirectory();
        if (topdir.exists() && topdir.isDirectory())
        {
            dirs.add (topdir.getChildFile ("libs/lua-kv/src").getFullPathName());
            dirs.add (topdir.getChildFile ("libs/element/lua").getFullPathName());
        }
#endif
        return dirs;
    }

    static String getApplicationLuaDir()
    {
        return DataPath::applicationDataDir().getChildFile ("Modules").getFullPathName();
    }

    static File getSystemLuaDir()
    {
        File dir;
#if defined(EL_APPIMAGE)
        dir = getAppImageLuaPath().getFullPathName();

#elif defined(EL_LUADIR)
        if (File::isAbsolutePath (EL_LUADIR))
            dir = File (EL_LUADIR);

#elif JUCE_MAC
        dir = File::getSpecialLocation (File::currentApplicationFile)
                  .getChildFile ("Contents/Resources/Modules")
                  .getFullPathName();

#elif JUCE_WINDOWS
        const auto installDir = DataPath::installDir();
        if (installDir.isDirectory())
            dir = installDir.getChildFile ("Modules").getFullPathName();
#endif

        return dir;
    }

    //==============================================================================
    static String getScriptSearchPath()
    {
        if (auto* scriptPath = std::getenv ("ELEMENT_SCRIPTS_PATH"))
            return String::fromUTF8 (scriptPath).trim();

        StringArray dirs;
        dirs.add (getLocalScriptsDir());
        dirs.add (getUserScriptsDir());
        dirs.add (getApplicationScriptsDir());
        dirs.add (getSystemScriptsDir());
        dirs.removeEmptyStrings();
        dirs.removeDuplicates (false);

        StringArray path;
        for (const auto& dir : dirs)
        {
            path.add (dir + "/?.lua");
        }

        jassert (path.size() > 1);
        return path.joinIntoString (";");
    }

    static String getLuaPath()
    {
        if (auto* luaPath = std::getenv ("LUA_PATH"))
            return String::fromUTF8 (luaPath).trim();

        StringArray dirs = getLocalLuaDirs();
        dirs.add (getApplicationLuaDir());
        dirs.add (getSystemLuaDir().getFullPathName());
        dirs.removeEmptyStrings();
        dirs.removeDuplicates (false);

        StringArray path;

        if (dirs.size() <= 0)
        {
#if defined(LUA_PATH_DEFAULT)
            path.addArray (StringArray::fromTokens (LUA_PATH_DEFAULT, ";"));
#endif
        }
        else
        {
            for (const auto& dir : dirs)
            {
                path.add (dir + "/?.lua");
                path.add (dir + "/?/init.lua");
            }
        }

        jassert (path.size() > 0);
        return path.joinIntoString (";");
    }

    static String getLuaCPath()
    {
        if (auto* luaPath = std::getenv ("LUA_CPATH"))
            return String::fromUTF8 (luaPath).trim();

        StringArray dirs;

        dirs.removeDuplicates (true);
        dirs.removeEmptyStrings();

        StringArray path;
        for (const auto& dir : dirs)
        {
            path.add (dir + "/?.so");
            path.add (dir + "/loadall.so");
        }

        return path.joinIntoString (";");
    }

    //==============================================================================
    static int searchInternalModules (lua_State* L)
    {
        const auto mod = sol::stack::get<std::string> (L);

        if (mod == "el.CommandManager")
        {
            sol::stack::push (L, luaopen_el_CommandManager);
        }
        else if (mod == "el.Globals")
        {
            sol::stack::push (L, luaopen_el_Globals);
        }
        else if (mod == "el.MidiPipe")
        {
            sol::stack::push (L, luaopen_el_MidiPipe);
        }
        else if (mod == "el.Node")
        {
            sol::stack::push (L, luaopen_el_Node);
        }
        else if (mod == "el.Session")
        {
            sol::stack::push (L, luaopen_el_Session);
        }

#define EL_LUA_INTERNAL_MOD_KV 1
#if defined(EL_LUA_INTERNAL_MOD_KV)
        else if (mod == "kv.audio")
        {
            sol::stack::push (L, luaopen_kv_audio);
        }
        else if (mod == "kv.midi")
        {
            sol::stack::push (L, luaopen_kv_midi);
        }
        else if (mod == "kv.bytes")
        {
            sol::stack::push (L, luaopen_kv_bytes);
        }
        else if (mod == "kv.round")
        {
            sol::stack::push (L, luaopen_kv_round);
        }
        else if (mod == "kv.AudioBuffer32")
        {
            sol::stack::push (L, luaopen_kv_AudioBuffer32);
        }
        else if (mod == "kv.AudioBuffer64")
        {
            sol::stack::push (L, luaopen_kv_AudioBuffer64);
        }
        else if (mod == "kv.MidiMessage")
        {
            sol::stack::push (L, luaopen_kv_MidiMessage);
        }
        else if (mod == "kv.MidiBuffer")
        {
            sol::stack::push (L, luaopen_kv_MidiBuffer);
        }
        else if (mod == "kv.Graphics")
        {
            sol::stack::push (L, luaopen_kv_Graphics);
        }
        else if (mod == "kv.Point")
        {
            sol::stack::push (L, luaopen_kv_Point);
        }
        else if (mod == "kv.Range")
        {
            sol::stack::push (L, luaopen_kv_Range);
        }
        else if (mod == "kv.Rectangle")
        {
            sol::stack::push (L, luaopen_kv_Rectangle);
        }

        else if (mod == "kv.Bounds")
        {
            sol::stack::push (L, luaopen_kv_Bounds);
        }
        else if (mod == "kv.TextButton")
        {
            sol::stack::push (L, luaopen_kv_TextButton);
        }
        else if (mod == "kv.Widget")
        {
            sol::stack::push (L, luaopen_kv_Widget);
        }
        else if (mod == "kv.Desktop")
        {
            sol::stack::push (L, luaopen_kv_Desktop);
        }
        else if (mod == "kv.DocumentWindow")
        {
            sol::stack::push (L, luaopen_kv_DocumentWindow);
        }
        else if (mod == "kv.MouseEvent")
        {
            sol::stack::push (L, luaopen_kv_MouseEvent);
        }
        else if (mod == "kv.File")
        {
            sol::stack::push (L, luaopen_kv_File);
        }
        else if (mod == "kv.Slider")
        {
            sol::stack::push (L, luaopen_kv_Slider);
        }
#endif

        else
        {
            String msg = "no internal '";
            msg << mod << "'";
            sol::stack::push (L, msg.toStdString());
        }

        return 1;
    }

    //==============================================================================
    void setGlobals (sol::state_view& view, Globals& g)
    {
        view.globals().set ("el.globals", std::ref<Globals> (g));
    }

    void clearGlobals (sol::state_view& view)
    {
        view.globals().set ("el.globals", sol::lua_nil);
    }

    //==============================================================================
    void initializeState (sol::state_view& view)
    {
        view.open_libraries();

        auto package = view["package"];
        auto newSearchers = view.create_table();
        newSearchers.add (package["searchers"][1]);
        newSearchers.add (searchInternalModules);
        sol::table packageSearchers = package["searchers"];
        for (int i = 2; i <= packageSearchers.size(); ++i)
            newSearchers.add (package["searchers"][i]);
        package["searchers"] = newSearchers;

        package["path"] = getLuaPath().toStdString();
        package["cpath"] = getLuaCPath().toStdString();
        package["spath"] = getScriptSearchPath().toStdString();
    }

    void initializeState (sol::state_view& view, Globals& g)
    {
        initializeState (view);
        setGlobals (view, g);
    }

} // namespace Lua
} // namespace Element
