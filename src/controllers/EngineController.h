#include "controllers/AppController.h"
#include "session/Node.h"

namespace Element {

struct ConnectionBuilder;
class GraphController;
class RootGraphController;
    
class EngineController : public AppController::Child,
                         private ChangeListener
{
public:
    EngineController();
    ~EngineController();
    
    /** sync models with engine */
    void syncModels();
    
    /** activate the controller */
    void activate() override;
    
    /** deactivate the controller */
    void deactivate() override;
    
    /** Adds a new node to the current graph. */
    void addNode (const Node& node);
    
    /** Adds a new node to a specificied graph */
    Node addNode (const Node& node, const Node& target, const ConnectionBuilder&);

    /** Adds a plugin by description to the current graph */
    void addPlugin (const PluginDescription& desc, const bool verified = true, const float rx = 0.5f, const float ry = 0.5f);
    
    /** Adds a plugin to a specific graph */
    void addPlugin (const Node& graph, const PluginDescription& desc);
    
    /** Adds a plugin to a specific graph and adds connections from
        a ConnectionBuilder */
    Node addPlugin (const Node& graph, const PluginDescription& desc, const ConnectionBuilder& builder, const bool verified = true);

    /** Adds a midi device node to the current root graph */
    void addMidiDeviceNode (const String& device, const bool isInput);

    /** Removes a node from the current graph */
    void removeNode (const uint32);
    
    /** remove a node by object */
    void removeNode (const Node& node);

    /** remove a node by Uuid */
    void removeNode (const Uuid&);
    
    /** Adds a new root graph */
    void addGraph();
    
    /** adds a specific graph */
    void addGraph (const Node& n);

    /** Remove a root graph by index */
    void removeGraph (int index = -1);

    /** Duplicates the currently active root graph */
    void duplicateGraph();

    /** Ads a specific new graph */
    void duplicateGraph (const Node& graph);
    
    /** Add a connection on the active root graph */
    void addConnection (const uint32, const uint32, const uint32, const uint32);

    /** Add a connection on a specific graph */
    void addConnection (const uint32 s, const uint32 sp, 
                        const uint32 d, const uint32 dp, const Node& graph);

    /** Connect by channel on the root graph */
    void connectChannels (const uint32, const int, const uint32, const int);

    /** Remove a connection on the active root graph */
    void removeConnection (const uint32, const uint32, const uint32, const uint32);

    /** Remove a connection on the specified graph */
    void removeConnection (const uint32, const uint32, const uint32, const uint32, const Node& target);

    /** Disconnect the provided node */
    void disconnectNode (const Node& node, const bool inputs = true, const bool outputs = true);

    /** Clear the root graph */
    void clear();
    
    /** Change root node */
    void setRootNode (const Node&);
    
    /** called when the session loads or re-loads */
    void sessionReloaded();
    
    /** replace a node with a given plugin */
    void replace (const Node&, const PluginDescription&);
    
    void changeBusesLayout (const Node& node, const AudioProcessor::BusesLayout& layout);
    
private:
    friend struct RootGraphHolder;
    class RootGraphs; friend class RootGraphs;
    ScopedPointer<RootGraphs> graphs;
    
    friend class ChangeBroadcaster;
    void changeListenerCallback (ChangeBroadcaster*) override;
    Node addPlugin (GraphController& controller, const PluginDescription& desc);
};
    
}
