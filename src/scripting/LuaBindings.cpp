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
#include "engine/audioengine.hpp"
#include "engine/midipipe.hpp"
#include "gui/SystemTray.h"
#include "scripting/ScriptManager.h"
#include "session/commandmanager.hpp"
#include "session/node.hpp"
#include "session/pluginmanager.hpp"
#include "session/presetmanager.hpp"
#include "session/session.hpp"
#include "datapath.hpp"
#include "globals.hpp"
#include "settings.hpp"
#include "sol/sol.hpp"
#include "el/lua-kv.hpp"

#include "libelement.a.p/AudioBuffer.h"
#include "libelement.a.p/command.h"
#include "libelement.a.p/object.h"
#include "libelement.a.p/script.h"
#include "libelement.a.p/slug.h"

extern "C" {
extern int luaopen_el_audio (lua_State* L);
extern int luaopen_el_bytes (lua_State*);
extern int luaopen_el_midi (lua_State*);
extern int luaopen_el_round (lua_State*);
extern int luaopen_el_AudioBuffer32 (lua_State*);
extern int luaopen_el_AudioBuffer64 (lua_State*);
extern int luaopen_el_Bounds (lua_State*);
extern int luaopen_el_TextButton (lua_State*);
extern int luaopen_el_Widget (lua_State*);
extern int luaopen_el_Desktop (lua_State*);
extern int luaopen_el_DocumentWindow (lua_State*);
extern int luaopen_el_File (lua_State*);
extern int luaopen_el_Graphics (lua_State*);
extern int luaopen_el_MidiBuffer (lua_State*);
extern int luaopen_el_MidiMessage (lua_State*);
extern int luaopen_el_MouseEvent (lua_State*);
extern int luaopen_el_Point (lua_State*);
extern int luaopen_el_Range (lua_State*);
extern int luaopen_el_Rectangle (lua_State*);
extern int luaopen_el_Slider (lua_State*);
extern int luaopen_el_MidiPipe (lua_State*);
extern int luaopen_el_CommandManager (lua_State*);
extern int luaopen_el_Globals (lua_State*);
extern int luaopen_el_Node (lua_State*);
extern int luaopen_el_Session (lua_State*);
}

using namespace sol;

namespace Element {
namespace Lua {

    //==============================================================================
#if defined(EL_APPIMAGE)
    static File getAppImageLuaPath()
    {
        return File::getSpecialLocation (File::currentExecutableFile)
            .getParentDirectory() // bin
            .getParentDirectory() // usr
            .getChildFile ("share/element/modules");
    }
#endif

    //==============================================================================
    static String getLocalScriptsDir()
    {
        File dir;
        return dir.exists() ? dir.getFullPathName() : String();
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
    static StringArray getLocalLuaDirs()
    {
        StringArray dirs;
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
        auto windowsDir = File::getSpecialLocation (File::currentExecutableFile)
                              .getParentDirectory()
                              .getChildFile ("lua");

        if (windowsDir.exists() && windowsDir.isDirectory())
        {
            dir = windowsDir.getFullPathName();
        }
        else
        {
            windowsDir = DataPath::installDir();
            if (windowsDir.exists() && windowsDir.isDirectory())
                dir = windowsDir.getChildFile ("lua").getFullPathName();
        }
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
#define DEFINE_LUA_TXT_LOADER(pkgname)                                                              \
    static int load_el_##pkgname (lua_State* L)                                                     \
    {                                                                                               \
        sol::state_view view (L);                                                                   \
        sol::stack::push (L, view.script (BinaryData::pkgname##_lua, (String ("el.") + #pkgname).toStdString())); \
        return 1;                                                                                   \
    }

    DEFINE_LUA_TXT_LOADER (AudioBuffer)
    DEFINE_LUA_TXT_LOADER (command)
    DEFINE_LUA_TXT_LOADER (object)
    DEFINE_LUA_TXT_LOADER (script)
    DEFINE_LUA_TXT_LOADER (slug)
#undef DEFINE_LUA_TXT_LOADER

    static int searchInternalModules (lua_State* L)
    {
        const auto mod = sol::stack::get<std::string> (L);
        if (mod == "el.AudioBuffer")
        {
            sol::stack::push (L, load_el_AudioBuffer);
        }
        else if (mod == "el.command")
        {
            sol::stack::push (L, load_el_command);
        }
        else if (mod == "el.object")
        {
            sol::stack::push (L, load_el_object);
        }
        else if (mod == "el.script")
        {
            sol::stack::push (L, load_el_script);
        }
        else if (mod == "el.slug")
        {
            sol::stack::push (L, load_el_slug);
        }

        else if (mod == "el.CommandManager")
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
        else if (mod == "el.audio" || mod == "kv.audio")
        {
            sol::stack::push (L, luaopen_el_audio);
        }
        else if (mod == "el.midi" || mod == "kv.midi")
        {
            sol::stack::push (L, luaopen_el_midi);
        }
        else if (mod == "el.bytes" || mod == "kv.bytes")
        {
            sol::stack::push (L, luaopen_el_bytes);
        }
        else if (mod == "el.round" || mod == "kv.round")
        {
            sol::stack::push (L, luaopen_el_round);
        }
        else if (mod == "el.AudioBuffer32" || mod == "kv.AudioBuffer32")
        {
            sol::stack::push (L, luaopen_el_AudioBuffer32);
        }
        else if (mod == "el.AudioBuffer64" || mod == "kv.AudioBuffer64")
        {
            sol::stack::push (L, luaopen_el_AudioBuffer64);
        }
        else if (mod == "el.MidiMessage" || mod == "kv.MidiMessage")
        {
            sol::stack::push (L, luaopen_el_MidiMessage);
        }
        else if (mod == "el.MidiBuffer" || mod == "kv.MidiBuffer")
        {
            sol::stack::push (L, luaopen_el_MidiBuffer);
        }
        else if (mod == "el.Graphics" || mod == "kv.Graphics")
        {
            sol::stack::push (L, luaopen_el_Graphics);
        }
        else if (mod == "el.Point" || mod == "kv.Point")
        {
            sol::stack::push (L, luaopen_el_Point);
        }
        else if (mod == "el.Range" || mod == "kv.Range")
        {
            sol::stack::push (L, luaopen_el_Range);
        }
        else if (mod == "el.Rectangle" || mod == "kv.Rectangle")
        {
            sol::stack::push (L, luaopen_el_Rectangle);
        }

        else if (mod == "el.Bounds" || mod == "kv.Bounds")
        {
            sol::stack::push (L, luaopen_el_Bounds);
        }
        else if (mod == "el.TextButton" || mod == "kv.TextButton")
        {
            sol::stack::push (L, luaopen_el_TextButton);
        }
        else if (mod == "el.Widget" || mod == "kv.Widget")
        {
            sol::stack::push (L, luaopen_el_Widget);
        }
        else if (mod == "el.Desktop" || mod == "kv.Desktop")
        {
            sol::stack::push (L, luaopen_el_Desktop);
        }
        else if (mod == "el.DocumentWindow" || mod == "kv.DocumentWindow")
        {
            sol::stack::push (L, luaopen_el_DocumentWindow);
        }
        else if (mod == "el.MouseEvent" || mod == "kv.MouseEvent")
        {
            sol::stack::push (L, luaopen_el_MouseEvent);
        }
        else if (mod == "el.File" || mod == "kv.File")
        {
            sol::stack::push (L, luaopen_el_File);
        }
        else if (mod == "el.Slider" || mod == "kv.Slider")
        {
            sol::stack::push (L, luaopen_el_Slider);
        }
#endif

        else
        {
            std::stringstream msg;
            msg << "no internal '" << mod << "'";
            sol::stack::push (L, msg.str());
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
