
#include "controllers/EngineController.h"
#include "Signals.h"
#include "Messages.h"

namespace Element {

void AddPluginMessage::createActions (AppController& app, OwnedArray<UndoableAction>& actions) const
{
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

    actions.add (new AddPluginAction (app, *this));
}

void RemoveNodeMessage::createActions (AppController& app,
                                       OwnedArray<UndoableAction>& actions) const
{
    class RemoveNodeAction : public UndoableAction
    {
    public:
        explicit RemoveNodeAction (AppController& a, const Node& node)
            : app(a), nodeUuid (node.getUuid()), targetGraph (node.getParentGraph())
        {
            node.getArcs (arcs);
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
 
            DBG("[EL] graph undo remove node: " << targetGraph.getName());

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

        bool isDataValid() const {
            return targetGraph.isGraph() && !nodeUuid.isNull() && nodeData.isValid();
        }
    };

    if (node.isValid())
        actions.add (new RemoveNodeAction (app, node));
    for (const auto& n : nodes)
        actions.add (new RemoveNodeAction (app, n));
}

}
