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

#include "scripting/LuaIterators.h"

#include "sol/sol.hpp"
#include "lua-kv.h"

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

extern void openJUCE (sol::state&);
extern void bindJUCE (sol::table&);

static auto NS (state& lua, const char* name) { return lua[name].get_or_create<table>(); }

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
        "invoke",           &CommandManager::invoke,

        /// Invoke a command directly
        // @int Command ID
        // @bool async
        // @function invokeDirectly
        // @treturn bool True if success
        "invokeDirectly",   &CommandManager::invokeDirectly
    );

    C.new_usertype<DeviceManager> ("DeviceManager", no_constructor);
    C.new_usertype<MappingEngine> ("MappingEngine", no_constructor);
    C.new_usertype<MidiEngine> ("MidiEngine", no_constructor);
    C.new_usertype<PluginManager> ("PluginManager", no_constructor);
    C.new_usertype<PresetCollection> ("PresetCollection", no_constructor);
    C.new_usertype<Settings> ("Settings", no_constructor);

    /// A collection of global objects 
    // @type World
    auto W = C.new_usertype<Globals> ("World", no_constructor,
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
    W.set_function ("audioengine", [&world]() { return world.getAudioEngine(); });
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

template<typename T>
static auto addRectangle (sol::table& view, const char* name)
{
    using R = Rectangle<T>;
    
    return view.new_usertype<R> (name, 
        sol::constructors<R(), R(T, T, T, T), R(T, T), R(Point<T>, Point<T>)>(),
        
        sol::meta_method::to_string, [](R& self) {
            return self.toString().toStdString();
        },

        "from_coords",      R::leftTopRightBottom,

        "x",                sol::property (&R::getX, &R::setX),
        "y",                sol::property (&R::getY, &R::setY),
        "width",            sol::property (&R::getWidth, &R::setWidth),
        "height",           sol::property (&R::getHeight, &R::setHeight),

        "left",             sol::property (&R::getX, &R::setLeft),
        "right",            sol::property (&R::getRight, &R::setRight),
        "top",              sol::property (&R::getY, &R::setTop),
        "bottom",           sol::property (&R::getBottom, &R::setBottom),

        "is_empty",         &R::isEmpty,
        "is_finite",        &R::isFinite,
        "translate",        &R::translate,
        "translated",       &R::translated,

        "expand",           &R::expand,
        "expanded", sol::overload (
            [](R& self, T dx, T dy) { return self.expanded (dx, dy); },
            [](R& self, T d)        { return self.expanded (d); }
        ),
        
        "reduce",               &R::reduce,
        "reduced", sol::overload (
            [](R& self, T dx, T dy) { return self.reduced (dx, dy); },
            [](R& self, T d)        { return self.reduced (d); }
        ),

        "to_int",           &R::toNearestInt,
        "to_int_edges",     &R::toNearestIntEdges
#if 0
        ,

        "getCentreX",       &R::getCentreX,
        "getCentreY",       &R::getCentreY,
        "getCentre",        &R::getCentre,
        "getAspectRatio", sol::overload (
            [](R& self) { return self.getAspectRatio(); },
            [](R& self, bool widthOverHeight) { return self.getAspectRatio (widthOverHeight); }
        ),

        "get_position",     &R::getPosition,
        "set_position", sol::overload (
            sol::resolve<void(Point<T>)> (&R::setPosition),
            sol::resolve<void(T, T)> (&R::setPosition)
        ),

        "getTopLeft",           &R::getTopLeft,
        "getTopRight",          &R::getTopRight,
        "getBottomLeft",        &R::getBottomLeft,
        "getBottomRight",       &R::getBottomRight,
        "getHorizontalRange",   &R::getHorizontalRange,
        "getVerticalRange",     &R::getVerticalRange,
        "setSize",              &R::setSize,
        "setBounds",            &R::setBounds,
        "setX",                 &R::setX,
        "setY",                 &R::setY,
        "setWidth",             &R::setWidth,
        "setHeight",            &R::setHeight,
        "setCentre", sol::overload (
            sol::resolve<void(T, T)> (&R::setCentre),
            sol::resolve<void(Point<T>)> (&R::setCentre)
        ),
        "setHorizontalRange",   &R::setHorizontalRange,
        "setVerticalRange",     &R::setVerticalRange,
        "withX",                &R::withX,
        "withY",                &R::withY,
        "withRightX",           &R::withRightX,
        "withBottomY",          &R::withBottomY,
        "withPosition", sol::overload (
            [](R& self, T x, T y)       { return self.withPosition (x, y); },
            [](R& self, Point<T> pt)    { return self.withPosition (pt); }
        ),
        "withZeroOrigin",       &R::withZeroOrigin,
        "withCentre",           &R::withCentre,

        "withWidth",            &R::withWidth,
        "withHeight",           &R::withHeight,
        "withSize",             &R::withSize,
        "withSizeKeepingCentre",&R::withSizeKeepingCentre,
        "setLeft",              &R::setLeft,
        "withLeft",             &R::withLeft,

        "setTop",               &R::setTop,
        "withTop",              &R::withTop,
        "setRight",             &R::setRight,
        "withRight",            &R::withRight,
        "setBottom",            &R::setBottom,
        "withBottom",           &R::withBottom,
        "withTrimmedLeft",      &R::withTrimmedLeft,
        "withTrimmedRight",     &R::withTrimmedRight,
        "withTrimmedTop",       &R::withTrimmedTop,
        "withTrimmedBottom",    &R::withTrimmedBottom,
        
        "slice_top",        &R::removeFromTop,
        "slice_left",       &R::removeFromLeft,
        "slice_right",      &R::removeFromRight,
        "slice_bottom",     &R::removeFromBottom,

        "getConstrainedPoint",  &R::getConstrainedPoint,

        "getRelativePoint",     [](R& self, T rx, T ry) { return self.getRelativePoint (rx, ry); },
        "proportionOfWidth",    [](R& self, T p)        { return self.proportionOfWidth (p); },
        "proportionOfHeight",   [](R& self, T p)        { return self.proportionOfHeight (p); },
        "getProportion",        [](R& self, R pr)       { return self.getProportion (pr); }
#if 0
        "",     &R::,
#endif
#endif
    );
}

class ComponentWrapper : public Component
{
public:
    ~ComponentWrapper()
    {
        widget = sol::lua_nil;
    }

    static ComponentWrapper* create (sol::table obj)
    {
        auto* wrapper = new ComponentWrapper();
        wrapper->widget = obj;
        wrapper->resized();
        wrapper->repaint();
        return wrapper;
    }

    void resized() override
    {
        if (sol::safe_function f = widget ["resized"])
            f (widget);
    }

    void paint (Graphics& g) override
    {
        if (sol::safe_function f = widget ["paint"]) {
            f (widget, std::ref<Graphics> (g));
        }
    }

    void mouseDrag (const MouseEvent& ev) override
    {
        if (sol::safe_function f = widget ["mouse_drag"])
            f (widget, ev);
    }

    void mouseDown (const MouseEvent& ev) override
    {
        if (sol::safe_function f = widget ["mouse_down"])
            f (widget, ev);
    }

    void mouseUp (const MouseEvent& ev) override
    {
        if (sol::safe_function f = widget ["mouse_up"])
            f (widget, ev);
    }

    void add (sol::table child, int zorder)
    {
        if (Component* const impl = child.get<Component*> ("impl"))
            addAndMakeVisible (*impl, zorder);
    }
    
    sol::table getBoundsTable()
    {
        sol::state_view L (widget.lua_state());
        auto r = getBounds();
        auto t = L.create_table();
        t["x"]      = r.getX();
        t["y"]      = r.getY();
        t["width"]  = r.getWidth();
        t["height"] = r.getHeight();
        return t;
    }

private:
    ComponentWrapper() = default;

    sol::table widget;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ComponentWrapper)
};

class WindowWrapper : public DocumentWindow
{
public:
    ~WindowWrapper()
    {
        widget = sol::lua_nil;
    }

    static WindowWrapper* create (sol::table tbl)
    {
        auto* wrapper = new WindowWrapper();
        wrapper->widget = tbl;
        return wrapper;
    }

    void closeButtonPressed() override
    {
        if (sol::safe_function f = widget ["onclosebutton"])
            f (widget);
    }

private:
    explicit WindowWrapper (const String& name = "Window")
        : DocumentWindow (name, Colours::darkgrey, DocumentWindow::allButtons, false)
    {
        setUsingNativeTitleBar (true);
        setResizable (true, false);
    }

    sol::table widget;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WindowWrapper)
};

//=============================================================================
EL_EXPORT int luaopen_el_File (lua_State* L)
{
    sol::state_view lua (L);
    auto t = lua.create_table();
    t.new_usertype<File> ("File", sol::constructors<File()>(),
        "name", sol::readonly_property ([](File& self) {
            return self.getFileName().toStdString();
        })
    );

    auto M = t.get<sol::table> ("File");
    t.clear();
    sol::stack::push (L, M);
    return 1;
}

//=============================================================================
EL_EXPORT int luaopen_element_ui (lua_State* L)
{
    sol::state_view lua (L);
    sol::table M = lua.create_table();

    addRectangle<lua_Number>  (M, "Rectangle");
    addRectangle<lua_Integer> (M, "IRectangle");

    /// A drawing context.
    // @type Context
    M.new_usertype<Graphics> ("Graphics", sol::no_constructor,
        /// Change the color
        // @function set_color
        // @int color New ARGB color as integer. e.g.`0xAARRGGBB`
        "set_color", sol::overload (
            [](Graphics& g, int color) { g.setColour (Colour (color)); }
        ),
        
        /// Draw some text
        // @function draw_text
        // @string text Text to draw
        // @int x Horizontal position
        // @int y Vertical position
        // @int width Width of containing area
        // @int height Height of containing area
        "draw_text", sol::overload (
            [](Graphics& g, std::string t, int x, int y, int w, int h) {
                g.drawText (t, x, y, w, h, Justification::centred, true);
            }
        ),

        /// Fill the entire drawing area.
        // @function fill_all
        "fill_all", sol::overload (
            [](Graphics& g)                 { g.fillAll(); },
            [](Graphics& g, int color)      { g.fillAll (Colour (color)); }
        )
    );

    /// A pair of x,y coordinates.
    // @type Point
    using PTF = Point<lua_Number>;
    M.new_usertype<PTF> ("Point", no_constructor,
        sol::call_constructor, sol::factories (
            []() { return PTF(); },
            [](lua_Number x, lua_Number y) { return PTF (x, y); }
        ),
        sol::meta_method::to_string, [](PTF& self) {
            return self.toString().toStdString();
        },

        /// True if is the origin point
        // @function is_origin
        "is_origin",    &PTF::isOrigin,

        /// True if is finite
        // @function is_finite
        "is_finite",    &PTF::isFinite,

        /// X coord
        // @class field
        // @name x
        "x",            sol::property (&PTF::getX, &PTF::setX),

        /// Y coord
        // @class field
        // @name x
        "y",            sol::property (&PTF::getY, &PTF::setY),

        /// True if is finite
        // @function with_x
        "with_x",       &PTF::withX,

        /// True if is finite
        // @function with_y
        "with_y",       &PTF::withY,

        /// True if is finite
        // @function set_xy
        "set_xy",       &PTF::setXY,

        /// True if is finite
        // @function add_xy
        "add_xy",       &PTF::addXY,

        /// True if is finite
        // @function translated
        "translated",   &PTF::translated,

        /// True if is finite
        // @function distance
        "distance", sol::overload (
            [](PTF& self) { return self.getDistanceFromOrigin(); },
            [](PTF& self, PTF& o) { return self.getDistanceFrom (o); }
        ),

        /// True if is finite
        // @function distance_squared
        "distance_squared", sol::overload (
            [](PTF& self) { return self.getDistanceSquaredFromOrigin(); },
            [](PTF& self, PTF& o) { return self.getDistanceSquaredFrom (o); }
        ),

        /// True if is finite
        // @function angle_to
        "angle_to",     &PTF::getAngleToPoint,

        /// True if is finite
        // @function rotated
        "rotated",      &PTF::rotatedAboutOrigin,
        
        /// True if is finite
        // @function dot_product
        "dot_product",  &PTF::getDotProduct,
        
        /// True if is finite
        // @function to_int
        "to_int",       &PTF::toInt
    );

    /// A mouse event
    // @type MouseEvent
    M.new_usertype<MouseEvent> ("MouseEvent", sol::no_constructor,
        "position", sol::readonly_property ([](MouseEvent& self) {
            return self.position.toDouble();
        }),
        "x",            &MouseEvent::x,
        "y",            &MouseEvent::y,
        "pressure",     &MouseEvent::pressure,
        "orientation",  &MouseEvent::orientation,
        "rotation",     &MouseEvent::rotation,
        "tiltx",        &MouseEvent::tiltX,
        "tilty",        &MouseEvent::tiltY
    );
    
    /// Implementation type used by el.Widget
    M.new_usertype<ComponentWrapper> ("ComponentWrapper",
        sol::no_constructor,
        sol::call_constructor, sol::factories (ComponentWrapper::create),

        "get_name",           [](ComponentWrapper& self) { return self.getName().toStdString(); },
        "set_name",           [](ComponentWrapper& self, const char* name) { self.setName (name); },
        "set_size",             &ComponentWrapper::setSize,
        "set_visible",          &ComponentWrapper::setVisible,
        "repaint",            [](ComponentWrapper& self) { self.repaint(); },
        "is_visible",           &ComponentWrapper::isVisible,
        "get_width",            &ComponentWrapper::getWidth,
        "get_height",           &ComponentWrapper::getHeight,
        "add_to_desktop",     [](ComponentWrapper& self) { self.addToDesktop (0); },
        "remove_from_desktop",  &ComponentWrapper::removeFromDesktop,
        "is_on_desktop",        &ComponentWrapper::isOnDesktop,
        "add",                  &ComponentWrapper::add,
        "resize",               &ComponentWrapper::setSize,
        "set_bounds",         [](ComponentWrapper& self, int x, int y, int w, int h) {
            self.setBounds (x, y, w, h);
        },
        "get_bounds",           &ComponentWrapper::getBoundsTable,

        sol::base_classes,  sol::bases<juce::Component, juce::MouseListener>()
    );

    M.new_usertype<WindowWrapper> ("WindowWrapper",
        sol::no_constructor,
        "create",               WindowWrapper::create,
        "get_name",             [](WindowWrapper& self) { return self.getName().toStdString(); },
        "set_name",             [](WindowWrapper& self, const char* name) { self.setName (name); },
        "set_size",             &WindowWrapper::setSize,
        "set_visible",          &WindowWrapper::setVisible,
        "repaint",              [](WindowWrapper& self) { self.repaint(); },
        "is_visible",           &WindowWrapper::isVisible,
        "get_width",            &WindowWrapper::getWidth,
        "get_height",           &WindowWrapper::getHeight,
        "add_to_desktop",       [](WindowWrapper& self) { self.addToDesktop(); },
        "remove_from_desktop",  &WindowWrapper::removeFromDesktop,
        "is_on_desktop",        &WindowWrapper::isOnDesktop,
        "set_content_owned",    &WindowWrapper::setContentOwned,
        sol::base_classes,  sol::bases<juce::DocumentWindow, juce::Component, juce::MouseListener>()
    );

    sol::stack::push (L, M);
    return 1;
}

EL_EXPORT int luaopen_juce (lua_State* L)
{
    sol::state_view lua (L);
    auto M = lua.create_table();
    bindJUCE (M);
    sol::stack::push (L, M);
    return 1;
}

static File scriptsDir()
{
    return File::getSpecialLocation (File::invokedExecutableFile)
                        .getParentDirectory().getParentDirectory().getParentDirectory()
                        .getChildFile ("scripts");
}

static File rootPath()
{
    return File::getSpecialLocation (File::invokedExecutableFile)
        .getParentDirectory().getParentDirectory().getParentDirectory();
}

static File defaultLuaPath()
{
    return File::getSpecialLocation (File::invokedExecutableFile)
                        .getParentDirectory().getParentDirectory().getParentDirectory()
                        .getChildFile ("libs/element/src");
}

static String getSearchPath()
{
    Array<File> paths ({
        rootPath().getChildFile ("libs/lua-kv/src"),
        rootPath().getChildFile ("libs/element/src")
    });
    
    StringArray path;
    for (const auto& dir : paths)
    {
        path.add (dir.getFullPathName() + "/?.lua");
        path.add (dir.getFullPathName() + "/?/init.lua");
    }

    return path.joinIntoString (";");
}

static int requireElement (lua_State* L)
{
    const auto mod = sol::stack::get<std::string> (L);
    if (mod == "el.File")
    {
        sol::stack::push (L, luaopen_el_File);
    }
	else if (mod == "element.ui")
	{
		sol::stack::push (L, luaopen_element_ui);
	}
    else if (mod == "juce")
    {
        sol::stack::push (L, luaopen_juce);
    }
    else
    {
	    sol::stack::push (L, "Not found");
    }
    
	return 1;
}

//=============================================================================
void openLibs (sol::state& lua)
{
    auto e = NS (lua, "element");
}


void initializeState (sol::state& lua, Globals& world)
{
    lua.open_libraries();
    
    auto searchers = lua["package"]["searchers"].get<sol::table>();
    searchers.add (requireElement);

    lua.globals().set ("element.world", std::ref<Globals> (world));

    lua["package"]["path"] = getSearchPath().toStdString();
    auto path = scriptsDir().getFullPathName(); path << "/?.lua";
    lua["package"]["spath"] = path.toStdString();

    lua.script ("_G['element'] = require ('element')");
    
    Lua::openWorld (world, lua);
    Lua::openModel (lua);
}

}}
