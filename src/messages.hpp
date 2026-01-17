// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce/core.hpp>
#include <element/juce/data_structures.hpp>

#include <element/controller.hpp>
#include <element/node.hpp>

namespace element {

class Services;
class ContentView;
class Context;

class Action : public juce::UndoableAction
{
public:
    virtual ~Action() {}

protected:
    Action() {}
};

struct AppMessage : public juce::Message
{
    enum ID
    {

    };

    inline virtual void createActions (Services&, juce::OwnedArray<juce::UndoableAction>&) const {}
};

struct AddMidiDeviceMessage : public AppMessage
{
    AddMidiDeviceMessage (const MidiDeviceInfo& dev, const bool isInput)
        : device (dev), inputDevice (isInput) {}
    const MidiDeviceInfo device;
    const bool inputDevice;
};

/** Send this to add a preset for a node */
struct AddPresetMessage : public AppMessage
{
    AddPresetMessage (const Node& n, const juce::String& name_ = String())
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
    RemoveNodeMessage (const NodeArray& n) : nodeId (EL_INVALID_NODE) { nodes.addArray (n); }
    RemoveNodeMessage (const uint32 _nodeId) : nodeId (_nodeId) {}
    const uint32 nodeId;
    const Node node;
    NodeArray nodes;

    virtual void createActions (Services& app, OwnedArray<UndoableAction>& actions) const;
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
        sourcePort = destPort = EL_INVALID_PORT;
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
    void createActions (Services& app, OwnedArray<UndoableAction>& actions) const override;
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
        sourcePort = destPort = EL_INVALID_PORT;
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
    void createActions (Services& app, OwnedArray<UndoableAction>& actions) const override;
};

class AddNodeMessage : public Message
{
public:
    AddNodeMessage (const Node& n, const Node& t = Node(), const File& f = File())
        : node (Node::resetIds (n.data().createCopy()), false),
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
    void createActions (Services& app, OwnedArray<UndoableAction>& actions) const override;
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
    std::function<void()> onFinished;
};

struct OpenSessionMessage : public AppMessage
{
    OpenSessionMessage (const File& f) : file (f) {}
    ~OpenSessionMessage() {}
    const File file;
};

//=============================================================================
struct RefreshControllerMessage : public AppMessage
{
    RefreshControllerMessage (const Controller& d)
        : device (d) {}
    ~RefreshControllerMessage() {}
    const Controller device;
};

struct AddControllerMessage : public AppMessage
{
    AddControllerMessage (const Controller& d)
        : device (d) {}
    AddControllerMessage (const File& f)
        : file (f) {}
    ~AddControllerMessage() noexcept {}
    const Controller device;
    const File file;
};

struct RemoveControllerMessage : public AppMessage
{
    RemoveControllerMessage (const Controller& d)
        : device (d) {}
    ~RemoveControllerMessage() noexcept {}
    const Controller device;
};

struct AddControlMessage : public AppMessage
{
    AddControlMessage (const Controller& d, const Control& c)
        : device (d), control (c) {}
    ~AddControlMessage() noexcept {}
    const Controller device;
    const Control control;
};

struct RemoveControlMessage : public AppMessage
{
    RemoveControlMessage (const Controller& d, const Control& c)
        : device (d), control (c) {}
    ~RemoveControlMessage() noexcept {}
    const Controller device;
    const Control control;
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

} // namespace element
