
#include "controllers/EngineController.h"
#include "Signals.h"
#include "Messages.h"

namespace Element {

class AddPluginAction : public UndoableAction
{
public:
    AddPluginAction (AppController& _app, const AddPluginMessage& msg) 
        : app (_app), graph (msg.graph), description (msg.description),
            builder (msg.builder), verified (msg.verified) { }
    ~AddPluginAction () noexcept { }

    bool perform() override
    {
        addedNode = Node();
        if (auto* ec = app.findChild<EngineController>())
            if (graph.isGraph())
                addedNode = ec->addPlugin (graph, description, builder, verified);

        return addedNode.isValid();
    }

    bool undo() override
    {
        if (! addedNode.isValid())
            return false;
        if (auto* ec = app.findChild<EngineController>())
            ec->removeNode (addedNode);
        addedNode = Node();
        return true;
    }

private:
    AppController& app;
    const Node graph;
    const PluginDescription description;
    const ConnectionBuilder builder;
    const bool verified = true;
    Node addedNode;
};

class RemoveNodeAction : public UndoableAction
{
public:
    explicit RemoveNodeAction (AppController& a, const Node& node)
        : app(a), nodeUuid (node.getUuid()), targetGraph (node.getParentGraph())
    {
        node.getArcs (arcs);
        Node mutableNode (node);
        mutableNode.savePluginState();
        node.getRelativePosition (x, y);
        nodeData = node.getValueTree().createCopy();
        Node::sanitizeRuntimeProperties (nodeData);
    }
    
    bool perform() override
    {
        auto& ec = *app.findChild<EngineController>();
        ec.removeNode (nodeUuid);
        return true;
    }
    
    bool undo() override
    {
        auto& ec = *app.findChild<EngineController>();
        bool handled = true;

        const Node newNode (nodeData, false);
        ec.addNode (newNode, targetGraph, builder);

        for (const auto* arc : arcs)
        {
            ec.addConnection (arc->sourceNode, arc->sourcePort,
                arc->destNode, arc->destPort, targetGraph);
        }

        return handled;
    }

private:
    AppController& app;
    ValueTree nodeData;
    const Node targetGraph;
    const Uuid nodeUuid;
    ConnectionBuilder builder;
    OwnedArray<Arc> arcs;
    double x = 0.5;
    double y = 0.5;
    bool isDataValid() const {
        return targetGraph.isGraph() && !nodeUuid.isNull() && nodeData.isValid();
    }
};

class AddConnectionAction : public UndoableAction
{
public:
    AddConnectionAction (AppController& a, const Node& targetGraph,
                                           const uint32 sn, const uint32 sp,
                                           const uint32 dn, const uint32 dp)
        : app (a), graph (targetGraph), arc (sn, sp, dn, dp)
    { }

    bool perform() override
    {
        auto& ec = *app.findChild<EngineController>();
        if (! graph.isValid())
            ec.addConnection (arc.sourceNode, arc.sourcePort, arc.destNode, arc.destPort);
        else
            ec.addConnection (arc.sourceNode, arc.sourcePort, arc.destNode, arc.destPort, graph);
        return true;
    }

    bool undo() override
    {
        auto& ec = *app.findChild<EngineController>();
        if (! graph.isValid())
            ec.removeConnection (arc.sourceNode, arc.sourcePort, arc.destNode, arc.destPort);
        else
            ec.removeConnection (arc.sourceNode, arc.sourcePort, arc.destNode, arc.destPort, graph);
        return true;
    }

private:
    AppController& app;
    const Node graph;
    const Arc arc;
};

class RemoveConnectionAction : public UndoableAction
{
public:
    RemoveConnectionAction (AppController& a, const Node& targetGraph,
                                              const uint32 sn, const uint32 sp,
                                              const uint32 dn, const uint32 dp)
        : app (a), graph (targetGraph), arc (sn, sp, dn, dp)
    { }

    bool perform() override
    {
        auto& ec = *app.findChild<EngineController>();
        if (! graph.isValid())
            ec.removeConnection (arc.sourceNode, arc.sourcePort, arc.destNode, arc.destPort);
        else
            ec.removeConnection (arc.sourceNode, arc.sourcePort, arc.destNode, arc.destPort, graph);
        return true;
    }

    bool undo() override
    {
        auto& ec = *app.findChild<EngineController>();
        if (! graph.isValid())
            ec.addConnection (arc.sourceNode, arc.sourcePort, arc.destNode, arc.destPort);
        else
            ec.addConnection (arc.sourceNode, arc.sourcePort, arc.destNode, arc.destPort, graph);
        return true;
    }

private:
    AppController& app;
    const Node graph;
    const Arc arc;
};

void AddPluginMessage::createActions (AppController& app, OwnedArray<UndoableAction>& actions) const
{
    actions.add (new AddPluginAction (app, *this));
}

void RemoveNodeMessage::createActions (AppController& app, OwnedArray<UndoableAction>& actions) const
{
    if (node.isValid())
        actions.add (new RemoveNodeAction (app, node));
    for (const auto& n : nodes)
        actions.add (new RemoveNodeAction (app, n));
}

void AddConnectionMessage::createActions (AppController& app, OwnedArray<UndoableAction>& actions) const
{
    jassert (usePorts()); // channel-ports not yet supported
    actions.add (new AddConnectionAction (app, target, sourceNode, sourcePort, destNode, destPort));
}

void RemoveConnectionMessage::createActions (AppController& app, OwnedArray<UndoableAction>& actions) const
{
    jassert (usePorts()); // channel-ports not yet supported
    actions.add (new RemoveConnectionAction (app, target, sourceNode, sourcePort, destNode, destPort));
}

}
