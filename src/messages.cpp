// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/context.hpp>
#include <element/engine.hpp>
#include <element/plugins.hpp>
#include <element/signals.hpp>

#include "messages.hpp"

namespace element {

class AddPluginAction : public UndoableAction
{
public:
    AddPluginAction (Services& _app, const AddPluginMessage& msg)
        : app (_app), graph (msg.graph), description (msg.description), builder (msg.builder), verified (msg.verified) {}
    ~AddPluginAction() noexcept {}

    bool perform() override
    {
        addedNode = Node();
        if (auto* ec = app.find<EngineService>())
            if (graph.isGraph())
                addedNode = addPlugin (*ec);
        return addedNode.isValid();
    }

    bool undo() override
    {
        if (! addedNode.isValid())
            return false;
        if (! havePosition)
            addedNode.getRelativePosition (x, y);
        havePosition = true;
        if (auto* ec = app.find<EngineService>())
            ec->removeNode (addedNode);
        addedNode = Node();
        return true;
    }

private:
    Services& app;
    const Node graph;
    const PluginDescription description;
    const ConnectionBuilder builder;
    const bool verified = true;
    double x, y;
    bool havePosition = false;
    Node addedNode;

    Node addPlugin (EngineService& ec)
    {
        auto node = app.context().plugins().getDefaultNode (description);
        if (! node.isValid())
            return ec.addPlugin (graph, description, builder, verified);
        return ec.addNode (node, graph, builder);
    }
};

class RemoveNodeAction : public UndoableAction
{
public:
    explicit RemoveNodeAction (Services& a, const Node& node)
        : app (a), targetGraph (node.getParentGraph()), nodeUuid (node.getUuid())
    {
        node.getArcs (arcs);
        Node mutableNode (node);
        mutableNode.savePluginState();
        node.getRelativePosition (x, y);
        nodeData = node.data().createCopy();
        Node::sanitizeRuntimeProperties (nodeData);
    }

    bool perform() override
    {
        auto& ec = *app.find<EngineService>();
        ec.removeNode (nodeUuid);
        return true;
    }

    bool undo() override
    {
        auto& ec = *app.find<EngineService>();
        bool handled = true;

        const Node newNode (nodeData, false);
        auto createdNode (ec.addNode (newNode, targetGraph, builder));
        createdNode.setRelativePosition (x, y); // TODO: GraphManager should handle this

        for (const auto* arc : arcs)
            ec.addConnection (arc->sourceNode, arc->sourcePort, arc->destNode, arc->destPort, targetGraph);

        return handled;
    }

private:
    Services& app;
    ValueTree nodeData;
    const Node targetGraph;
    const Uuid nodeUuid;
    ConnectionBuilder builder;
    OwnedArray<Arc> arcs;
    double x = 0.5;
    double y = 0.5;
    bool isDataValid() const
    {
        return targetGraph.isGraph() && ! nodeUuid.isNull() && nodeData.isValid();
    }
};

class AddConnectionAction : public UndoableAction
{
public:
    AddConnectionAction (Services& a, const Node& targetGraph, const uint32 sn, const uint32 sp, const uint32 dn, const uint32 dp)
        : app (a), graph (targetGraph), arc (sn, sp, dn, dp)
    {
    }

    bool perform() override
    {
        auto& ec = *app.find<EngineService>();
        if (! graph.isValid())
            ec.addConnection (arc.sourceNode, arc.sourcePort, arc.destNode, arc.destPort);
        else
            ec.addConnection (arc.sourceNode, arc.sourcePort, arc.destNode, arc.destPort, graph);
        return true;
    }

    bool undo() override
    {
        auto& ec = *app.find<EngineService>();
        if (! graph.isValid())
            ec.removeConnection (arc.sourceNode, arc.sourcePort, arc.destNode, arc.destPort);
        else
            ec.removeConnection (arc.sourceNode, arc.sourcePort, arc.destNode, arc.destPort, graph);
        return true;
    }

private:
    Services& app;
    const Node graph;
    const Arc arc;
};

class RemoveConnectionAction : public UndoableAction
{
public:
    RemoveConnectionAction (Services& a, const Node& targetGraph, const uint32 sn, const uint32 sp, const uint32 dn, const uint32 dp)
        : app (a), graph (targetGraph), arc (sn, sp, dn, dp)
    {
    }

    bool perform() override
    {
        auto& ec = *app.find<EngineService>();
        if (! graph.isValid())
            ec.removeConnection (arc.sourceNode, arc.sourcePort, arc.destNode, arc.destPort);
        else
            ec.removeConnection (arc.sourceNode, arc.sourcePort, arc.destNode, arc.destPort, graph);
        return true;
    }

    bool undo() override
    {
        auto& ec = *app.find<EngineService>();
        if (! graph.isValid())
            ec.addConnection (arc.sourceNode, arc.sourcePort, arc.destNode, arc.destPort);
        else
            ec.addConnection (arc.sourceNode, arc.sourcePort, arc.destNode, arc.destPort, graph);
        return true;
    }

private:
    Services& app;
    const Node graph;
    const Arc arc;
};

//=============================================================================

void AddPluginMessage::createActions (Services& app, OwnedArray<UndoableAction>& actions) const
{
    actions.add (new AddPluginAction (app, *this));
}

void RemoveNodeMessage::createActions (Services& app, OwnedArray<UndoableAction>& actions) const
{
    if (node.isValid())
        actions.add (new RemoveNodeAction (app, node));
    for (const auto& n : nodes)
        actions.add (new RemoveNodeAction (app, n));
}

void AddConnectionMessage::createActions (Services& app, OwnedArray<UndoableAction>& actions) const
{
    jassert (usePorts()); // channel-ports not yet supported
    actions.add (new AddConnectionAction (app, target, sourceNode, sourcePort, destNode, destPort));
}

void RemoveConnectionMessage::createActions (Services& app, OwnedArray<UndoableAction>& actions) const
{
    jassert (usePorts()); // channel-ports not yet supported
    actions.add (new RemoveConnectionAction (app, target, sourceNode, sourcePort, destNode, destPort));
}

} // namespace element
