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

/// @module element

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
#include "Globals.h"
#include "Settings.h"
#include "sol/sol.hpp"
#include "lua-kv.hpp"

extern "C" {
extern int luaopen_kv_audio (lua_State* L);
extern int luaopen_kv_byte (lua_State*);
extern int luaopen_kv_midi (lua_State*);
extern int luaopen_kv_round (lua_State*);
}

extern int luaopen_kv_AudioBuffer32 (lua_State*);
extern int luaopen_kv_AudioBuffer64 (lua_State*);
extern int luaopen_kv_Bounds (lua_State*);
extern int luaopen_kv_Component (lua_State*);
extern int luaopen_kv_DocumentWindow (lua_State*);
extern int luaopen_kv_File (lua_State*);
extern int luaopen_kv_Graphics (lua_State*);
extern int luaopen_kv_MidiBuffer (lua_State*);
extern int luaopen_kv_MidiMessage (lua_State*);
extern int luaopen_kv_MouseEvent (lua_State*);
extern int luaopen_kv_Point (lua_State*);
extern int luaopen_kv_Rectangle (lua_State*);
extern int luaopen_el_MidiPipe (lua_State*);

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

LUAMOD_API int luaopen_el_Session (lua_State* L) {
    sol::state_view lua (L);
    auto M = lua.create_table();
    M.new_usertype<Session> ("Session", no_constructor,
        meta_function::to_string, [](Session* self) {
            String str = "Session";
            if (self->getName().isNotEmpty())
                str << ": " << self->getName();
            return str.toStdString();
        },

        meta_function::length, [](Session* self) { return self->getNumGraphs(); },

        meta_function::index, [](Session* self, int index) {
            return isPositiveAndBelow (--index, self->getNumGraphs())
                ? std::make_shared<Node> (self->getGraph(index).getValueTree(), false)
                : std::shared_ptr<Node>();
        },

        "name", sol::property (
            [](Session& self, const char* name) -> void {
                self.setName (String::fromUTF8 (name));
            }, 
            [](Session& self) -> std::string {
                return self.getName().toStdString();
            }
        ),
        "toXmlString", [](Session *self) -> std::string {
            auto tree = self->getValueTree().createCopy();
            Node::sanitizeRuntimeProperties (tree, true);
            return tree.toXmlString().toStdString();
        }
        
       #if 0
        "clear",                    &Session::clear,
        "get_num_graphs",           &Session::getNumGraphs,
        "get_graph",                &Session::getGraph,
        "get_active_graph",         &Session::getActiveGraph,
        "get_active_graph_index",   &Session::getActiveGraphIndex,
        "add_graph",                &Session::addGraph,
        "save_state",               &Session::saveGraphState,
        "restore_state",            &Session::restoreGraphState
       #endif
    );

    sol::stack::push (L, kv::lua::remove_and_clear (M, "Session"));
    return 1;
}

LUAMOD_API int luaopen_el_Node (lua_State* L) {
    sol::state_view lua (L);
    auto M = lua.create_table();
    M.new_usertype<Node> ("Node", no_constructor,
        meta_function::to_string, [](const Node& self) -> std::string {
            String str = self.isGraph() ? "Graph" : "Node";
            if (self.getName().isNotEmpty())
                str << ": " << self.getName();
            return std::move (str.toStdString());
        },
        meta_function::length,  &Node::getNumNodes,
        meta_function::index,   [](Node* self, int index)
        {
            const auto child = self->getNode (index - 1);
            return child.isValid() ? std::make_shared<Node> (child.getValueTree(), false)
                                   : std::shared_ptr<Node>();
        },
        "valid",                readonly_property (&Node::isValid),
        "name", property (
            [](Node* self) { return self->getName().toStdString(); },
            [](Node* self, const char* name) { self->setProperty (Tags::name, String::fromUTF8 (name)); }
        ),
        "displayname",          readonly_property ([](Node* self) { return self->getDisplayName().toStdString(); }),
        "pluginname",           readonly_property ([](Node* self) { return self->getPluginName().toStdString(); }),
        "missing",              readonly_property (&Node::isMissing),
        "enabled",              readonly_property (&Node::isEnabled),
        "graph",                readonly_property (&Node::isGraph),
        "root",                 readonly_property (&Node::isRootGraph),
        "nodeid",               readonly_property (&Node::getNodeId),
        "uuid",                 readonly_property (&Node::getUuid),
        "uuidstring",           readonly_property (&Node::getUuidString),
        "type",                 readonly_property (&Node::getNodeType),
        "muted",                property (&Node::isMuted, &Node::setMuted),
        "bypassed",             readonly_property (&Node::isBypassed),
        "editor",               readonly_property (&Node::hasEditor),

        "toxmlstring", [](Node* self) -> std::string
        {
            auto copy = self->getValueTree().createCopy();
            Node::sanitizeRuntimeProperties (copy, true);
            return copy.toXmlString().toStdString();
        },
        "resetports",           &Node::resetPorts,
        "savestate",            &Node::savePluginState,
        "restoretate",          &Node::restorePluginState,
        "writefile", [](const Node& node, const char* filepath) -> bool {
            if (! File::isAbsolutePath (filepath))
                return false;
            return node.writeToFile (File (String::fromUTF8 (filepath)));
        }
        
       #if 0
        "has_modified_name",    &Node::hasModifiedName,
        "has_node_type",        &Node::hasNodeType,
        "get_parent_graph",     &Node::getParentGraph,
        "is_child_of_root_graph", &Node::isChildOfRootGraph,
        "is_muting_inputs",     &Node::isMutingInputs,
        "set_mute_input",       &Node::setMuteInput,
        "get_num_nodes",        &Node::getNumNodes,
        "get_node",             &Node::getNode,
       #endif
    );

    sol::stack::push (L, kv::lua::remove_and_clear (M, "Node"));
    return 1;
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
    
    // PortType
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
    
    // PortList
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

LUAMOD_API int luaopen_el_CommandManager (lua_State* L)
{
    sol::state_view lua (L);
    auto M = lua.create_table();
    /// Command Manager
    // @type CommandManager
    M.new_usertype<CommandManager> ("CommandManager", no_constructor,
        /// Invoke a command directly
        // @int command Command ID to invoke
        // @bool async Invoke now or aysnc
        // @function CommandManager:invoke_directly
        // @treturn bool True if success
        "invoke_directly",   &CommandManager::invokeDirectly
    );

    sol::stack::push (L, kv::lua::remove_and_clear (M, "CommandManager"));
    return 1;
}

static int el_Globals_instance (lua_State* L) {
    sol::state_view lua (L);
    auto& _G = lua.globals();
    sol::stack::push (L, _G.get_or<sol::userdata> ("el.globals", sol::lua_nil));
    return 1;
}

LUAMOD_API int luaopen_el_Globals (lua_State* L)
{
    using Element::Globals;
    sol::state_view lua (L);
    auto M = lua.create_table();
    /// A collection of global objects 
    // @type Globals
    M.new_usertype<Globals> ("Globals", sol::no_constructor,
        "instance",      el_Globals_instance,
        /// Get the current audio engine
        // @function audioengine
        // @treturn el.AudioEngine
        "audio",          &Globals::getAudioEngine,
        "commands",       &Globals::getCommandManager,
        "devices",        &Globals::getDeviceManager,
        "mappings",              &Globals::getMappingEngine,
        // "media_manager",        &Globals::getMediaManager,
        "midi",          &Globals::getMidiEngine,
        "plugins",       &Globals::getPluginManager,
        "presets",              &Globals::getPresetCollection,
        "session",              &Globals::getSession,
        "settings",             &Globals::getSettings
    );

    sol::stack::push (L, kv::lua::remove_and_clear (M, "Globals"));
    return 1;
}

static void openWorld (Globals& world, state& lua)
{
    auto e = lua["element"].get_or_create<sol::table>();
    auto C = e; // ["C"].get_or_create<sol::table>();

    C.new_usertype<AppController> ("AppController", no_constructor);
    C.new_usertype<GuiController> ("GuiController", no_constructor);
    C.new_usertype<AudioEngine> ("AudioEngine", no_constructor);
    C.new_usertype<DeviceManager> ("DeviceManager", no_constructor);
    C.new_usertype<MappingEngine> ("MappingEngine", no_constructor);
    C.new_usertype<MidiEngine> ("MidiEngine", no_constructor);
    C.new_usertype<PluginManager> ("PluginManager", no_constructor);
    C.new_usertype<PresetCollection> ("PresetCollection", no_constructor);
    C.new_usertype<Settings> ("Settings", no_constructor);
}

void openDSP (sol::state& lua)
{
    // kv_openlibs (lua.lua_state(), 0);
}

//=============================================================================
EL_EXPORT int luaopen_element_ui (lua_State* L)
{
    sol::state_view lua (L);
    sol::table M = lua.create_table();
    sol::stack::push (L, M);
    return 1;
}

EL_EXPORT int luaopen_juce (lua_State* L)
{
    sol::state_view lua (L);
    auto M = lua.create_table();
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
    else if (mod == "el.Globals")
    {
        sol::stack::push (L, luaopen_el_Globals);
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
    else if (mod == "kv.Component")
    {
        sol::stack::push (L, luaopen_kv_Component);
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
#endif

    else
    {
	    sol::stack::push (L, "Not found");
    }
    
	return 1;
}

//=============================================================================
void openLibs (sol::state& lua) {}

void initializeState (sol::state& lua) {
    lua.open_libraries ();
    
    auto searchers = lua["package"]["searchers"].get<sol::table>();
    searchers.add (searcher);

    lua["package"]["path"] = getSearchPath().toStdString();
    auto path = scriptsDir().getFullPathName(); path << "/?.lua";
    lua["package"]["spath"] = path.toStdString();
    DBG(getRootPath().getFullPathName());
}

void initializeState (sol::state& lua, Globals& world)
{
    initializeState (lua);
    lua.globals().set ("el.globals", std::ref<Globals> (world));
    lua.script ("_G['element'] = require ('element')");
}

}}
