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

#include "scripting/LuaIterators.h"

#include "sol/sol.hpp"
#include "lua-kv.h"

//=============================================================================
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

#define CALL(x) sol::c_call<decltype(x), x>
#define WRAP(x) sol::wrap<decltype(x), x>

using namespace sol;

namespace Element {
namespace Lua {

static auto NS (state& lua, const char* name) { return lua[name].get_or_create<table>(); }

template<typename T>
static auto addRange (state_view& view, const char* name)
{
    using R = Range<T>;
    return view.new_usertype<R> (name, no_constructor,
        call_constructor, factories (
            []() { return R(); },
            [] (T start, T end) { return R (start, end); }
        ),
        "empty",            readonly_property (&R::isEmpty),
        "start",            property (&R::getStart,  &R::setStart),
        "length",           property (&R::getLength, &R::setLength),
        "end",              property (&R::getEnd,    &R::setEnd),
        "clip",             &R::clipValue,
        "contains",         [](R* self, R* other) { return self->contains (*other); },
        "intersects",       [](R* self, R* other) { return self->intersects (*other); },
        "expanded",         &R::expanded
    );
}

template<typename T>
static auto addRectangle (state& lua, const char* ns, const char* name)
{
    using R = Rectangle<T>;
    auto view = NS (lua, ns);
    return view.new_usertype<R> (name, no_constructor,
        call_constructor, factories (
            []() { return R(); },
            [] (T w, T h) { return R (w, h); },
            [] (T x, T y, T w, T h) { return R (x, y, w, h); }
        ),
        meta_method::to_string, [](R* self) {
            return self->toString().toStdString();
        },
        "empty",            readonly_property (&R::isEmpty),
        "x",                property (&R::getX,  &R::setX),
        "y",                property (&R::getY,  &R::setY),
        "w",                property (&R::getWidth, &R::setWidth),
        "h",                property (&R::getHeight, &R::setHeight)
    );
}

template<typename T>
static auto addRect (sol::table view, const char* name)
{
    using R = Rectangle<T>;
    return view.new_usertype<R> (name, no_constructor,
        call_constructor, factories (
            []() { return R(); },
            [] (T w, T h) { return R (w, h); },
            [] (T x, T y, T w, T h) { return R (x, y, w, h); }
        ),
        meta_method::to_string, [](R* self) {
            return self->toString().toStdString();
        },
        "empty",            readonly_property (&R::isEmpty),
        "x",                property (&R::getX,  &R::setX),
        "y",                property (&R::getY,  &R::setY),
        "w",                property (&R::getWidth, &R::setWidth),
        "h",                property (&R::getHeight, &R::setHeight)
    );
}

static void openJUCE (state& lua)
{
    addRange<float>     (lua, "Range");
    addRange<int>       (lua, "Span");
    
    // AudioBuffer
    lua.new_usertype<AudioSampleBuffer> ("AudioBuffer", no_constructor,
        "cleared",      readonly_property (&AudioSampleBuffer::hasBeenCleared),
        "nchannels",    readonly_property (&AudioSampleBuffer::getNumChannels),
        "nframes",      readonly_property (&AudioSampleBuffer::getNumSamples),
        "resize", overload (
            [](AudioSampleBuffer& self, int nc, int ns) { self.setSize (nc, ns); },
            [](AudioSampleBuffer& self, int nc, int ns, bool keep) { self.setSize (nc, ns, keep); },
            [](AudioSampleBuffer& self, int nc, int ns, bool keep, bool clear) { self.setSize (nc, ns, keep, clear); },
            [](AudioSampleBuffer& self, int nc, int ns, bool keep, bool clear, bool avoid) { self.setSize (nc, ns, keep, clear, avoid); }),
        "duplicate", overload (
            [](AudioSampleBuffer& self, const AudioSampleBuffer& other) { self.makeCopyOf (other); },
            [](AudioSampleBuffer& self, const AudioSampleBuffer& other, bool avoidReallocate) { self.makeCopyOf (other, avoidReallocate); }),
        "clear", overload (
            resolve<void()> (&AudioSampleBuffer::clear),
            resolve<void(int,int)> (&AudioSampleBuffer::clear),
            resolve<void(int,int,int)> (&AudioSampleBuffer::clear)),
        "get_sample",       &AudioSampleBuffer::getSample,
        "set_sample",       CALL(&AudioSampleBuffer::setSample),
        "add_sample",       &AudioSampleBuffer::addSample,
        "apply_gain", overload (
            resolve<void(int,int,int,float)> (&AudioSampleBuffer::applyGain),
            resolve<void(int,int,float)> (&AudioSampleBuffer::applyGain),
            resolve<void(float)> (&AudioSampleBuffer::applyGain)),
        "apply_gain_ramp", overload (
            resolve<void(int,int,int,float,float)> (&AudioSampleBuffer::applyGainRamp),
            resolve<void(int,int,float,float)> (&AudioSampleBuffer::applyGainRamp)),
        "add_from", overload (
            [](AudioSampleBuffer& self, int dc, int dss, AudioSampleBuffer& src, int sc, int sss, int ns) {
                self.addFrom (dc, dss, src, sc, sss, ns);
            },
            [](AudioSampleBuffer& self, int dc, int dss, AudioSampleBuffer& src, int sc, int sss, int ns, float gain) {
                self.addFrom (dc, dss, src, sc, sss, ns, gain);
            }),
        "add_from_with_ramp",   &AudioSampleBuffer::addFromWithRamp,
        "copy_from", overload (
            resolve<void(int,int,const AudioSampleBuffer&,int,int,int)> (&AudioSampleBuffer::copyFrom),
            resolve<void(int,int,const float*, int)> (&AudioSampleBuffer::copyFrom),
            resolve<void(int,int,const float*, int, float)> (&AudioSampleBuffer::copyFrom)),
        "copy_from_with_ramp",  &AudioSampleBuffer::copyFromWithRamp,
        "find_min_max",         &AudioSampleBuffer::findMinMax,
        "magnitude", overload (
            [](const AudioSampleBuffer& self, int c, int s, int n) { return self.getMagnitude (c, s, n); },
            [](const AudioSampleBuffer& self, int s, int n) { return self.getMagnitude (s, n); }),
        "rms",                  &AudioSampleBuffer::getRMSLevel,
        "reverse",overload (
            [](const AudioSampleBuffer& self, int c, int s, int n) { return self.reverse (c, s, n); },
            [](const AudioSampleBuffer& self, int s, int n) { return self.reverse (s, n); })
    );
}

void openUI (state& lua)
{
    // addRectangle<int> (lua, "ui", "Bounds");
    auto systray = lua.new_usertype<SystemTray> ("systray", no_constructor,
        "enabled", sol::property (
            []() -> bool { return SystemTray::getInstance() != nullptr; },
            SystemTray::setEnabled));
}

static void openModel (sol::state& lua)
{
    auto e = NS (lua, "element");

    auto session = e.new_usertype<Session> ("Session", no_constructor,
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

        "name", property ([](Session* self, const char* name) -> void {
                self->setName (String::fromUTF8 (name));
            },[](const Session& self) -> std::string {
                return self.getName().toStdString();
            }),
        "toxmlstring", [](Session *self) -> std::string {
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

    auto node = e.new_usertype<Node> ("Node", no_constructor,
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

    addRectangle<double> (lua, "element", "Rect");
}

static void openWorld (Globals& world, state& lua)
{
    auto e = lua["element"].get_or_create<sol::table>();
    auto C = e; // ["C"].get_or_create<sol::table>();

    C.new_usertype<AppController> ("AppController", no_constructor);
    C.new_usertype<GuiController> ("GuiController", no_constructor);
    C.new_usertype<AudioEngine> ("AudioEngine", no_constructor);

    /// Command Manager
    // @type CommandManager

    C.new_usertype<CommandManager> ("CommandManager", no_constructor,
        /// Invoke a command 
        // @tparam 'element.CommandInfo' info
        // @bool async
        // @function invoke
        // @treturn bool True if success
        "invoke",          &CommandManager::invoke,

        /// Invoke a command directly
        // @int Command ID
        // @bool async
        // @function invoke_directly
        // @treturn bool True if success
        "invoke_directly", &CommandManager::invokeDirectly
    );

    C.new_usertype<DeviceManager> ("DeviceManager", no_constructor);
    C.new_usertype<MappingEngine> ("MappingEngine", no_constructor);
    C.new_usertype<MidiEngine> ("MidiEngine", no_constructor);
    C.new_usertype<PluginManager> ("PluginManager", no_constructor);
    C.new_usertype<PresetCollection> ("PresetCollection", no_constructor);
    C.new_usertype<Settings> ("Settings", no_constructor);

    /// A collection of global objects 
    // @type World
    C.new_usertype<Globals> ("World", no_constructor,
        /// Get the current audio engine
        // @function audioengine
        // @treturn element.AudioEngine
        "audioengine",      &Globals::getAudioEngine,
        "commands",         &Globals::getCommandManager,
        "devices",          &Globals::getDeviceManager,
        "mappings",         &Globals::getMappingEngine,
        "media",            &Globals::getMediaManager,
        "midiengine",       &Globals::getMidiEngine,
        "plugins",          &Globals::getPluginManager,
        "presets",          &Globals::getPresetCollection,
        "session",          &Globals::getSession,
        "settings",         &Globals::getSettings
    );
}

void openDSP (sol::state& lua)
{
    kv_openlibs (lua.lua_state(), 0);
}

#ifdef __cplusplus
 #define EL_EXTERN extern "C"
#else
 #define EL_EXTERN
#endif

#ifdef _WIN32
 #define EL_EXPORT EL_EXTERN __declspec(dllexport)
#else
 #define EL_EXPORT EL_EXTERN __attribute__((visibility("default")))
#endif

//=============================================================================
EL_EXPORT int luaopen_element_ui (lua_State* L)
{
    sol::state_view lua (L);
    sol::table M = lua.create_table();
    addRect<int> (M, "Rect");
    sol::stack::push (L, M);
    return 1;
}

static File scriptsDir()
{
    return File::getSpecialLocation (File::invokedExecutableFile)
                        .getParentDirectory().getParentDirectory().getParentDirectory()
                        .getChildFile ("scripts");
}

static File defaultLuaPath()
{
    return File::getSpecialLocation (File::invokedExecutableFile)
                        .getParentDirectory().getParentDirectory().getParentDirectory()
                        .getChildFile ("libs/element");
}

static int requireElement (lua_State* L)
{
    const String mod = sol::stack::get<std::string> (L);

    #if 0
	if (path == "element.ui")
	{
        DBG("found");
		sol::stack::push (L, luaopen_element_ui);
		return 1;
	}
    #else
    
    auto path = mod.replaceCharacter('.', '/');
    path << ".lua";
    auto file = scriptsDir().getChildFile(path);
    DBG(file.getFullPathName());
    if (file.existsAsFile()) {
        luaL_loadfile (L, file.getFullPathName().toRawUTF8());
        return 1;
    }

    #endif

	sol::stack::push (L, "Not found");
	return 1;
}

static int element_wrap (lua_State* L) {
    sol::state_view lua (L);
    sol::table M = lua.create_table();
    
    const auto module = sol::stack::get<std::string> (L);

    if (module.size() > 0)
    {
        M.set_function ("test", []() { DBG("hello world"); });
        addRect<int> (M, "Rect");
    }

    sol::stack::push (L, M);
    return 1;
}

//=============================================================================
void openLibs (sol::state& lua)
{
    auto e = NS (lua, "element");
    // e["wrap"] = element_wrap;
    // 

   #if 0
    openWorld (lua);
    openModel (lua);
    openDSP (lua);
    openKV (lua);
    openUI (lua);
   #endif
}

#include <memory>

void initializeState (sol::state& lua, Globals& world)
{
    lua.open_libraries();
    
    // auto searchers = lua["package"]["searchers"].get<sol::table>();
    // searchers.add (requireElement);

    lua.globals().set ("element.world", std::ref<Globals> (world));
    String path = String(defaultLuaPath().getFullPathName() + "/?.lua").toStdString();
    path << ";" << String(defaultLuaPath().getFullPathName() + "/?/init.lua").toStdString();
    lua["package"]["path"] = path.toStdString();
    path = scriptsDir().getFullPathName(); path << "/?.lua";
    lua["package"]["spath"] = path.toStdString();

    lua.script ("_G['element'] = require ('element')");
    Lua::openWorld (world, lua);
    Lua::openModel (lua);
}

}}
