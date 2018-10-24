
#include "controllers/EngineController.h"
#include "Signals.h"
#include "Messages.h"

namespace Element {

UndoableAction* AddPluginMessage::createUndoableAction (AppController& app) const
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

    return new AddPluginAction (app, *this);
}

UndoableAction* RemoveNodeMessage::createUndoableAction (AppController& app) const
{
    class RemoveNodeAction : public UndoableAction
    {
    public:
        RemoveNodeAction (AppController& a, const RemoveNodeMessage& msg)
            : app(a), node (msg.node), graph(msg.node.getParentGraph()),
              nodeId (msg.nodeId)
        {
            nodes.addArray (msg.nodes);
            node.getArcs (arcs);
            for (const auto& n : nodes)
            {
                n.getArcs (arcs);
                graphs.add (n.getParentGraph());
            }
        }
        
        bool perform() override
        {
            auto& ec = *app.findChild<EngineController>();

            bool handled = true;
            if (nodes.isEmpty())
            {
                if (node.isValid())
                    ec.removeNode (node);
                else if (node.getParentGraph().isRootGraph())
                    ec.removeNode (nodeId);
                else
                    handled = false;
            }
            else
            {
                NodeArray graphs;
                for (const auto& node : nodes)
                {
                    if (node.isRootGraph())
                        graphs.add (node);
                    else if (node.isValid())
                        ec.removeNode (node);
                }

                for (const auto& graph : graphs)
                {
                    // noop
                    ignoreUnused (graph);
                }
            }

            return handled;
        }
        
        bool undo() override
        {
            auto& ec = *app.findChild<EngineController>();
            bool handled = true;
            if (nodes.isEmpty())
            {
                ec.addNode (node, graph, builder);
            }
            else
            {
                jassert (graphs.size() >= nodes.size());
                for (int i = 0; i < nodes.size(); ++i)
                {
                    ec.addNode (nodes.getUnchecked(i),
                                graphs.getUnchecked(i), 
                                builder);
                }
            }
 
            DBG("[EL] graph undo remove node: " << graph.getName());

            for (const auto* arc : arcs)
            {
                const auto g (findGraph (*arc));
                if (g.isGraph())
                    ec.addConnection (arc->sourceNode, arc->sourcePort,
                        arc->destNode, arc->destPort, g);
            }

            return handled;
        }

    private:
        AppController& app;
        Node graph, node;
        NodeArray nodes, graphs;
        uint32 nodeId;
        ConnectionBuilder builder;
        OwnedArray<Arc> arcs;

        const Node findGraph (const Arc& arc)
        {
            if (graph.isValid())
                return graph;
            if (graphs.size() > 0)
                return graphs.getFirst();
            return Node();
        }
    };

    return new RemoveNodeAction (app, *this);
}

}
