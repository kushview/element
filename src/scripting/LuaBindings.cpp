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

#include "session/CommandManager.h"
#include "session/MediaManager.h"
#include "session/Node.h"
#include "session/PluginManager.h"
#include "session/Presets.h"
#include "session/Session.h"

#include "Settings.h"
#include "sol/sol.hpp"
#include "lua-kv.hpp"

namespace sol {
/** Support juce::ReferenceCountedObjectPtr */
template <typename T>
struct unique_usertype_traits<ReferenceCountedObjectPtr<T>> {
    typedef T type;
    typedef ReferenceCountedObjectPtr<T> actual_type;
    static const bool value = true;
    static bool is_null (const actual_type& ptr)    { return ptr == nullptr; }
    static type* get (const actual_type& ptr)       { return ptr.get(); }
};
}

#include "../../element/lua/el/CommandManager.cpp"
#include "../../element/lua/el/Globals.cpp"
#include "../../element/lua/el/Node.cpp"
#include "../../element/lua/el/Session.cpp"

extern "C" {
extern int luaopen_kv_audio (lua_State* L);
extern int luaopen_kv_byte (lua_State*);
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
extern int luaopen_kv_Rectangle (lua_State*);
extern int luaopen_kv_Slider (lua_State*);
extern int luaopen_el_MidiPipe (lua_State*);


using namespace sol;

namespace Element {
namespace Lua {

static auto NS (state& lua, const char* name) { return lua[name].get_or_create<table>(); }

void openUI (state& lua)
{
    auto systray = lua.new_usertype<SystemTray> ("systray", no_constructor,
        "enabled", sol::property (
            []() -> bool { return SystemTray::getInstance() != nullptr; },
            SystemTray::setEnabled));
}

static void openModel (sol::state& lua)
{
    auto e = NS (lua, "element");
    e.set_function ("newgraph", [](sol::variadic_args args) {
        String name;
        bool defaultGraph = false;
        int argIdx = 0;
        
        for (const auto arg : args)
        {
            if (arg.get_type() == sol::type::string && name.isNotEmpty())
                name = String::fromUTF8 (arg.as<const char*>());
            else if (arg.get_type() == sol::type::boolean)
                defaultGraph = arg.as<bool>();
            if (++argIdx == 2)
                break;
        }

        return defaultGraph ? Node::createDefaultGraph (name)
                            : Node::createGraph (name);
    });
}

void openKV (state& lua)
{
    auto kv   = NS (lua, "element");
    
    kv.new_usertype<kv::PortType> ("PortType", no_constructor,
        call_constructor, factories (
            [](int t) {
                if (t < 0 || t > kv::PortType::Unknown)
                    t = kv::PortType::Unknown;
                return kv::PortType (t);
            },
            [](const char* slug) {
                return kv::PortType (String::fromUTF8 (slug));
            }
        ),
        meta_method::to_string, [](PortType* self) {
            return self->getName().toStdString();
        },

        "name", readonly_property ([](kv::PortType* self) { return self->getName().toStdString(); }),
        "slug", readonly_property ([](kv::PortType* self) { return self->getSlug().toStdString(); }),
        "uri",  readonly_property ([](kv::PortType* self) { return self->getURI().toStdString(); })
    );

    kv.new_usertype<kv::PortDescription> ("PortDescription", no_constructor);
    
    kv.new_usertype<kv::PortList> ("PortList",
        sol::constructors<kv::PortList()>(),
        meta_method::to_string, [](MidiPipe*) { return "element.PortList"; },
        "add", [](kv::PortList* self, int type, int index, int channel,
                                      const char* symbol, const char* name,
                                      const bool input)
        {
            self->add (type, index, channel, symbol, name, input);
        }
    );
}


//=============================================================================
EL_EXPORT int luaopen_element_ui (lua_State* L)
{
    sol::state_view lua (L);
    sol::table M = lua.create_table();
    sol::stack::push (L, M);
    return 1;
}

static File getRootPath()
{
    return File::getCurrentWorkingDirectory();
}

static File scriptsDir()
{
    return getRootPath().getChildFile ("scripts");
}

static String getScriptSearchPath()
{
    Array<File> paths ({
        scriptsDir(),
        File::getSpecialLocation (File::userHomeDirectory).getChildFile(".local/lib/element/scripts"),
    });
    
    StringArray path;

    for (const auto& dir : paths)
    {
        path.add (dir.getFullPathName() + "/?.lua");
    }

    return path.joinIntoString (";");
}

static String getCSearchPath()
{
    Array<File> paths ({
        getRootPath().getChildFile ("build/lib/lua")
    });
    
    StringArray path;
    for (const auto& dir : paths)
    {
       #if JUCE_WINDOWS
        path.add (dir.getFullPathName() + "/?.dll");
        path.add (dir.getFullPathName() + "/loadall.dll");
       #else
        path.add (dir.getFullPathName() + "/?.so");
        path.add (dir.getFullPathName() + "/loadall.so");
       #endif
    }

    return path.joinIntoString (";");
}

static String getSearchPath()
{
    Array<File> paths ({
        getRootPath().getChildFile ("libs/lua-kv/src"),
        getRootPath().getChildFile ("libs/element/lua")
    });
    
    StringArray path;
    for (const auto& dir : paths)
    {
        path.add (dir.getFullPathName() + "/?.lua");
        path.add (dir.getFullPathName() + "/?/init.lua");
    }

    return path.joinIntoString (";");
}

static int searcher (lua_State* L)
{
    const auto mod = sol::stack::get<std::string> (L);

    if (mod == "element.ui")
	{
		sol::stack::push (L, luaopen_element_ui);
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
    
    
#define EL_LUA_INTERNAL_MOD_KV      1
#if defined (EL_LUA_INTERNAL_MOD_KV)
    else if (mod == "kv.audio")
    {
        sol::stack::push (L, luaopen_kv_audio);
    }
    else if (mod == "kv.midi")
    {
        sol::stack::push (L, luaopen_kv_midi);
    }
    else if (mod == "kv.byte")
    {
        sol::stack::push (L, luaopen_kv_byte);
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
	    sol::stack::push (L, "Not found");
    }
    
	return 1;
}

//==============================================================================
void initializeState (sol::state_view& lua)
{
    lua.open_libraries();
    
    auto package = lua["package"];
    
    auto searchers = lua.create_table();
    searchers.add (searcher);
    for (const auto& s : package["searchers"].get<sol::table>())
        searchers.add (s.second);
    
    package["searchers"]    = searchers;
    package["path"]         = getSearchPath().toStdString();
    package["cpath"]        = getCSearchPath().toStdString();
    package["spath"]        = getScriptSearchPath().toStdString();
}

void initializeState (sol::state_view& lua, Globals& world)
{
    initializeState (lua);
    lua.globals().set ("el.globals", std::ref<Globals> (world));
}

}}
