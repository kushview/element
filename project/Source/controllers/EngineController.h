#include "controllers/AppController.h"

namespace Element {
    
class RootGraphController;
class Node;
    
class EngineController : public AppController::Child,
                         private ChangeListener
{
public:
    EngineController();
    ~EngineController();
    
    /** activate the controller */
    void activate() override;
    
    /** deactivate the controller */
    void deactivate() override;
    
    /** Adds a new node to the current graph. */
    void addNode (const Node& node);
    
    /** Adds a plugin by description to the current graph */
    void addPlugin (const PluginDescription& desc, const bool verified = true, const float rx = 0.5f, const float ry = 0.5f);
    
    /** Removes a node from the current graph */
    void removeNode (const uint32);
    
    /** Adds a new root graph */
    void addGraph();
    
    /** Remove a root graph by index */
    void removeGraph (int index = -1);

    /** Duplicates the currently active root graph */
    void duplicateGraph();
    
    /** Add a subgraph to the currently active root graph */
    void addSubGraph();
    
    /** Add a connection on the active root graph */
    void addConnection (const uint32, const uint32, const uint32, const uint32);

    /** Connect by channel on the root graph */
    void connectChannels (const uint32, const int, const uint32, const int);

    /** Remove a connection on the active root graph */
    void removeConnection (const uint32, const uint32, const uint32, const uint32);

    /** Disconnect the provided node */
    void disconnectNode (const Node& node);

    /** Clear the root graph */
    void clear();
    
    /** Change root node */
    void setRootNode (const Node&);
    
    /** Updates the MIDI channel of a root graph by index */
    void updateRootGraphMidiChannel (const int index, const int midiChannel);
    
private:
    friend class RootGraphHolder;
    
    ScopedPointer<RootGraphController> root;
    
    class RootGraphs; friend class RootGraphs;
    ScopedPointer<RootGraphs> graphs;
    
    friend class ChangeBroadcaster;
    void changeListenerCallback (ChangeBroadcaster*) override;
    void addMissingIONodes();
};
    
}
