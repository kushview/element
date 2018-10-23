
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
              builder(), verified (msg.verified) { }
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
            DBG ("undo: " << addedNode.getName());
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

}