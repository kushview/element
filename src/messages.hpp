/*
    This file is part of Element
    Copyright (C) 2018-2019 Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "ElementApp.h"
#include "session/controllerdevice.hpp"
#include "session/node.hpp"

namespace element {

class AppController;
class ContentView;
class Context;

class Action : public UndoableAction
{
public:
    virtual ~Action() {}

protected:
    Action() {}
};

struct AppMessage : public Message
{
    enum ID
    {

    };

    inline virtual void createActions (AppController&, OwnedArray<UndoableAction>&) const {}
};

struct AddMidiDeviceMessage : public AppMessage
{
    AddMidiDeviceMessage (const String& name, const bool isInput)
        : device (name), inputDevice (isInput) {}
    const String device;
    const bool inputDevice;

    inline PluginDescription getPluginDescription() const
    {
        PluginDescription desc;
        desc.pluginFormatName = "Internal";
        desc.fileOrIdentifier = inputDevice ? "element.midiInputDevice" : "element.midiOutputDevice";
        desc.numInputChannels = desc.numOutputChannels = 0;
        desc.isInstrument = 0;
        return desc;
    }
};

/** Send this to add a preset for a node */
struct AddPresetMessage : public AppMessage
{
    AddPresetMessage (const Node& n, const String& name_ = String())
        : node (n), name (name_) {}
    ~AddPresetMessage() noexcept {}
    const Node node;
    const String name;
};

/** Send this to add a preset for a node */
struct SaveDefaultNodeMessage : public AppMessage
{
    SaveDefaultNodeMessage (const Node& n) : node (n) {}
   ~SaveDefaultNodeMessage() noexcept {}
    const Node node;
};

/** Send this to remove a node from the current graph */
struct RemoveNodeMessage : public AppMessage
{
    RemoveNodeMessage (const Node& n) : nodeId (n.getNodeId()), node (n) {}
    RemoveNodeMessage (const NodeArray& n) : nodeId (KV_INVALID_NODE) { nodes.addArray (n); }
    RemoveNodeMessage (const uint32 _nodeId) : nodeId (_nodeId) {}
    const uint32 nodeId;
    const Node node;
    NodeArray nodes;

    virtual void createActions (AppController& app, OwnedArray<UndoableAction>& actions) const;
};

/** Send this to add a new connection */
struct AddConnectionMessage : public AppMessage
{
    AddConnectionMessage (uint32 s, int sc, uint32 d, int dc, const Node& tgt = Node())
        : target (tgt)
    {
        sourceNode = s;
        destNode = d;
        sourceChannel = sc;
        destChannel = dc;
        sourcePort = destPort = KV_INVALID_PORT;
        jassert (useChannels());
    }

    AddConnectionMessage (uint32 s, uint32 sp, uint32 d, uint32 dp, const Node& tgt = Node())
        : target (tgt)
    {
        sourceNode = s;
        destNode = d;
        sourcePort = sp;
        destPort = dp;
        sourceChannel = destChannel = -1;
        jassert (usePorts());
    }

    uint32 sourceNode, sourcePort, destNode, destPort;
    int sourceChannel, destChannel;

    const Node target;

    inline bool useChannels() const { return sourceChannel >= 0 && destChannel >= 0; }
    inline bool usePorts() const { return ! useChannels(); }
    void createActions (AppController& app, OwnedArray<UndoableAction>& actions) const override;
};

/** Send this to remove a connection from the graph */
class RemoveConnectionMessage : public AppMessage
{
public:
    RemoveConnectionMessage (uint32 s, int sc, uint32 d, int dc, const Node& t = Node())
        : target (t)
    {
        sourceNode = s;
        destNode = d;
        sourceChannel = sc;
        destChannel = dc;
        sourcePort = destPort = KV_INVALID_PORT;
        jassert (useChannels());
    }

    RemoveConnectionMessage (uint32 s, uint32 sp, uint32 d, uint32 dp, const Node& t = Node())
        : target (t)
    {
        sourceNode = s;
        destNode = d;
        sourcePort = sp;
        destPort = dp;
        sourceChannel = destChannel = -1;
        jassert (usePorts());
    }

    uint32 sourceNode, sourcePort, destNode, destPort;
    int sourceChannel, destChannel;
    const Node target;

    inline bool useChannels() const { return sourceChannel >= 0 && destChannel >= 0; }
    inline bool usePorts() const { return ! useChannels(); }
    void createActions (AppController& app, OwnedArray<UndoableAction>& actions) const override;
};

class AddNodeMessage : public Message
{
public:
    AddNodeMessage (const Node& n, const Node& t = Node(), const File& f = File())
        : node (Node::resetIds (n.getValueTree().createCopy()), false),
          target (t),
          sourceFile (f)
    {
    }

    const Node node;
    const Node target;
    ConnectionBuilder builder;
    const File sourceFile;
};

/** Send this when a plugin needs loaded into the graph */
class LoadPluginMessage : public Message
{
public:
    LoadPluginMessage (const PluginDescription& pluginDescription, const bool pluginVerified)
        : Message(), description (pluginDescription), verified (pluginVerified) {}
    LoadPluginMessage (const PluginDescription& d, const bool v, const float rx, const float ry)
        : Message(), description (d), relativeX (rx), relativeY (ry), verified (v) {}
    ~LoadPluginMessage() {}

    /** Descriptoin of the plugin to load */
    const PluginDescription description;

    /** Relative X of the node UI in a graph editor */
    const float relativeX = 0.5f;

    /** Relative X of the node UI in a graph editor */
    const float relativeY = 0.5f;

    /** Whether or not this plugin has been vetted yet */
    const bool verified;
};

struct AddPluginMessage : public AppMessage
{
    AddPluginMessage (const Node& g, const PluginDescription& d, const bool v = true)
        : graph (g), description (d), verified (v)
    {
    }

    const Node graph;
    const PluginDescription description;
    const bool verified;
    ConnectionBuilder builder;
    void createActions (AppController& app, OwnedArray<UndoableAction>& actions) const override;
};

struct ReplaceNodeMessage : public AppMessage
{
    ReplaceNodeMessage (const Node& n, const PluginDescription& d, const bool v = true)
        : graph (n.getParentGraph()), node (n), description (d), verified (v) {}
    const Node graph;
    const Node node;
    const PluginDescription description;
    const bool verified;
    boost::signals2::signal<void()> success;
};

class DuplicateNodeMessage : public Message
{
public:
    DuplicateNodeMessage (const Node& n)
        : Message(), node (n) {}
    DuplicateNodeMessage() {}
    const Node node;
};

class DisconnectNodeMessage : public Message
{
public:
    DisconnectNodeMessage (const Node& n, const bool i = true, const bool o = true, const bool a = true, const bool m = true)
        : Message(), node (n), inputs (i), outputs (o), audio (a), midi (m) {}
    DisconnectNodeMessage()
        : Message(), inputs (true), outputs (true), audio (true), midi (true) {}
    const Node node;
    const bool inputs, outputs;
    const bool audio, midi;
};

struct FinishedLaunchingMessage : public AppMessage
{
    FinishedLaunchingMessage() {}
    ~FinishedLaunchingMessage() {}
};

struct ChangeBusesLayout : public AppMessage
{
    ChangeBusesLayout (const Node& n, const AudioProcessor::BusesLayout& l)
        : node (n), layout (l) {}
    const Node node;
    const AudioProcessor::BusesLayout layout;
};

struct OpenSessionMessage : public AppMessage
{
    OpenSessionMessage (const File& f) : file (f) {}
    ~OpenSessionMessage() {}
    const File file;
};

//=============================================================================
struct RefreshControllerDeviceMessage : public AppMessage
{
    RefreshControllerDeviceMessage (const ControllerDevice& d)
        : device (d) {}
    ~RefreshControllerDeviceMessage() {}
    const ControllerDevice device;
};

struct AddControllerDeviceMessage : public AppMessage
{
    AddControllerDeviceMessage (const ControllerDevice& d)
        : device (d) {}
    AddControllerDeviceMessage (const File& f)
        : file (f) {}
    ~AddControllerDeviceMessage() noexcept {}
    const ControllerDevice device;
    const File file;
};

struct RemoveControllerDeviceMessage : public AppMessage
{
    RemoveControllerDeviceMessage (const ControllerDevice& d)
        : device (d) {}
    ~RemoveControllerDeviceMessage() noexcept {}
    const ControllerDevice device;
};

struct AddControlMessage : public AppMessage
{
    AddControlMessage (const ControllerDevice& d, const ControllerDevice::Control& c)
        : device (d), control (c) {}
    ~AddControlMessage() noexcept {}
    const ControllerDevice device;
    const ControllerDevice::Control control;
};

struct RemoveControlMessage : public AppMessage
{
    RemoveControlMessage (const ControllerDevice& d, const ControllerDevice::Control& c)
        : device (d), control (c) {}
    ~RemoveControlMessage() noexcept {}
    const ControllerDevice device;
    const ControllerDevice::Control control;
};

struct RemoveControllerMapMessage : public AppMessage
{
    RemoveControllerMapMessage (const ControllerMap& mapp)
        : controllerMap (mapp) {}
    ~RemoveControllerMapMessage() noexcept {}
    const ControllerMap controllerMap;
};

//=============================================================================
struct WorkspaceOpenFileMessage : public AppMessage
{
    WorkspaceOpenFileMessage (const File& f) : file (f) {}
    ~WorkspaceOpenFileMessage() noexcept {}
    const File file;
};

struct ReloadMainContentMessage : public AppMessage
{
    explicit ReloadMainContentMessage (const String& t = String()) : type (t) {}
    ~ReloadMainContentMessage() noexcept {}
    const String type;
};

struct PresentViewMessage : public AppMessage
{
    using Factory = std::function<ContentView*()>;
    PresentViewMessage (Factory f) : create (f) {}
    PresentViewMessage() {}
    Factory create;
};

}
