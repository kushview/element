
#include "ElementApp.h"
#include "controllers/AppController.h"
#include "controllers/GuiController.h"
#include "controllers/GraphController.h"
#include "engine/SubGraphProcessor.h"
#include "session/DeviceManager.h"
#include "session/PluginManager.h"
#include "session/Node.h"
#include "Globals.h"
#include "Settings.h"

#include "controllers/EngineController.h"

namespace Element {
    
struct RootGraphHolder
{
    RootGraphHolder (const Node& n, Globals& world)
        : plugins(world.getPluginManager()), model (n)
    { }
    
    ~RootGraphHolder()
    {
        jassert(! attached());
        controller = nullptr;
        model.getValueTree().removeProperty (Tags::object, 0);
        node = nullptr;
        model = Node();
    }
    
    bool attached() const { return node && controller; }
    bool attach (AudioEnginePtr engine)
    {
        jassert (engine);
        if (! engine)
            return false;
        
        if (attached())
            return true;
        
        node = GraphNode::createForRoot (new RootGraph ());
        
        if (auto* root = getRootGraph())
        {
            if (engine->addGraph (root))
            {
                controller = new RootGraphController (*root, plugins);
                model.getValueTree().setProperty (Tags::object, node.get(), 0);
            }
        }
        
        return attached();
    }
    
    bool detach (AudioEnginePtr engine)
    {
        if (! engine)
            return false;
        
        if (! attached())
            return true;
        
        bool wasRemoved = false;
        if (auto* g = getRootGraph())
            wasRemoved = engine->removeGraph (g);
        
        if (wasRemoved)
        {
            controller = nullptr;
            model.getValueTree().removeProperty (Tags::object, 0);
            node = nullptr;
        }
        
        return wasRemoved;
    }
    
    RootGraphController* getController() const { return controller; }
    RootGraph* getRootGraph() const { return dynamic_cast<RootGraph*> (node ? node->getAudioProcessor() : nullptr); }
    
    bool hasController()    const { return nullptr != controller; }
    
    void addMissingIONodes()
    {
        auto* root = getController();
        if (! root) return;
        GraphNodePtr ioNodes [IOProcessor::numDeviceTypes];
        for (int i = 0; i < root->getNumFilters(); ++i)
        {
            GraphNodePtr node = root->getNode (i);
            if (node->isMidiIONode() || node->isAudioIONode())
            {
                auto* proc = dynamic_cast<IOProcessor*> (node->getAudioProcessor());
                ioNodes [proc->getType()] = node;
            }
        }
        
        for (int t = 0; t < IOProcessor::numDeviceTypes; ++t)
        {
            if (nullptr != ioNodes [t])
                continue;
            
            PluginDescription desc;
            desc.pluginFormatName = "Internal";
            double rx = 0.5f, ry = 0.5f;
            switch (t)
            {
                case IOProcessor::audioInputNode:
                    desc.fileOrIdentifier = "audio.input";
                    rx = .25;
                    ry = .25;
                    break;
                case IOProcessor::audioOutputNode:
                    desc.fileOrIdentifier = "audio.output";
                    rx = .25;
                    ry = .75;
                    break;
                case IOProcessor::midiInputNode:
                    desc.fileOrIdentifier = "midi.input";
                    rx = .75;
                    ry = .25;
                    break;
                case IOProcessor::midiOutputNode:
                    desc.fileOrIdentifier = "midi.output";
                    rx = .75;
                    ry = .75;
                    break;
            }
            
            auto nodeId = root->addFilter (&desc, rx, ry);
            ioNodes[t] = root->getNodeForId (nodeId);
            jassert(ioNodes[t] != nullptr);
        }
    }
    
    void resetIONodePorts()
    {
        const ValueTree nodes = model.getNodesValueTree();
        for (int i = nodes.getNumChildren(); --i >= 0;)
        {
            Node model (nodes.getChild (i), false);
            GraphNodePtr node = model.getGraphNode();
            if (node && (node->isAudioIONode() || node->isMidiIONode()))
                model.resetPorts();
        }
    }
    
private:
    friend class EngineController;
    friend class EngineController::RootGraphs;
    PluginManager&                      plugins;
    ScopedPointer<RootGraphController>  controller;
    Node                                model;
    GraphNodePtr                        node;
};

class EngineController::RootGraphs
{
public:
    RootGraphs (EngineController& e) : owner (e) { }
    ~RootGraphs() { }
    
    RootGraphHolder* add (RootGraphHolder* item)
    {
        jassert (! graphs.contains (item));
        return graphs.add (item);
    }
    
    void clear()
    {
        detachAll();
        graphs.clear();
    }
    
    /** This is recursive! */
    GraphController* findSubGraphController (GraphController* parent, const Node& n)
    {
        for (int i = parent->getNumFilters(); --i >= 0;)
        {
            if (GraphNodePtr node = parent->getNode (i))
            {
                if (auto* sub = dynamic_cast<SubGraphProcessor*> (node->getAudioProcessor()))
                {
                    if (sub->getController().isControlling (n))
                        return &sub->getController();
                    else if (auto* sub2 = findSubGraphController (&sub->getController(), n))
                        return sub2;
                }
            }
        }
        return nullptr;
    }

    GraphController* findSubGraphController (const Node& n)
    {
        if (n.isRootGraph() || !n.isGraph())
            return nullptr;
        
        for (auto* const h : graphs)
        {
            if (auto* controller = h->getController())
            {
                for (int i = controller->getNumFilters(); --i >= 0;)
                {
                    if (GraphNodePtr node = controller->getNode (i))
                        if (auto* sub = dynamic_cast<SubGraphProcessor*> (node->getAudioProcessor()))
                            if (sub->getController().isControlling (n))
                                return &sub->getController();
                }
            }
        }
        
        return nullptr;
    }

    RootGraphHolder* findByEngineIndex (const int index) const
    {
        if (index >= 0)
            for (auto* const n : graphs)
                if (auto* p = n->getRootGraph())
                    if (p->getEngineIndex() == index)
                        return n;
        return 0;
    }
    
    RootGraphHolder* findFor (const Node& node) const
    {
        for (auto* const n : graphs)
            if (n->model == node)
                return n;
        return 0;
    }
    
    /** Returns the active graph according to the engine */
    RootGraphHolder* findActiveInEngine() const
    {
        auto engine = owner.getWorld().getAudioEngine();
        if (! engine)
            return 0;
        const int currentIndex = engine->getActiveGraph();
        if (currentIndex >= 0)
            for (auto* h : graphs)
                if (auto* root = h->getRootGraph())
                    if (currentIndex == root->getEngineIndex())
                        return h;
        return 0;
    }
    
    /** Returns the active graph according to the session */
    RootGraphHolder* findActive() const
    {
        if (auto session = owner.getWorld().getSession())
            if (auto* h = findFor (session->getActiveGraph()))
                return h;
        return 0;
    }

    /** This returns a GraphController for the provided node. The
        passed in node is expected to have type="graph" 
        
        NOTE: this is a recursive operation
     */
    GraphController* findGraphControllerFor (const Node& graph)
    {
        for (const auto* h : graphs)
        {
            if (auto* controller = h->controller.get())
            {
                if (controller->isControlling (graph))
                    return controller;
                else if (auto* subController = findSubGraphController (controller, graph))
                    return subController;
            }
        }

        return nullptr;
    }

    RootGraphController* findActiveRootGraphController() const
    {
        if (auto* h = findActive())
            return h->controller.get();
        return 0;
    }
    
    void attachAll()
    {
        engine = owner.getWorld().getAudioEngine();
        for (auto* g : graphs)
            g->attach (engine);
    }
    
    void detachAll()
    {
        engine = owner.getWorld().getAudioEngine();
        for (auto* g : graphs)
            g->detach (engine);
    }
    
    // remove the holder, this will also delete it!
    void remove (RootGraphHolder* g)
    {
        graphs.removeObject (g, true);
    }
    
    void savePluginStates()
    {
        for (auto* h : graphs)
            h->model.savePluginState();
    }
    
    const OwnedArray<RootGraphHolder>& getGraphs() const { return graphs; }
    
private:
    EngineController& owner;
    SessionPtr session;
    EnginePtr  engine;
    OwnedArray<RootGraphHolder> graphs;
};

EngineController::EngineController()
    : AppController::Child()
{
    graphs = new RootGraphs (*this);
}

EngineController::~EngineController()
{
    graphs = nullptr;
}

void EngineController::addConnection (const uint32 s, const uint32 sp, const uint32 d, const uint32 dp)
{
    if (auto session = getWorld().getSession())
        if (auto *h = graphs->findFor (session->getCurrentGraph()))
            if (auto* c = h->getController())
                c->addConnection (s, sp, d, dp);
}

void EngineController::addConnection (const uint32 s, const uint32 sp, 
                                      const uint32 d, const uint32 dp, const Node& graph)
{
    if (auto* controller = graphs->findGraphControllerFor (graph))
        controller->addConnection (s, sp, d, dp);
}

void EngineController::addGraph()
{
    auto& world  = getWorld();
    auto engine  = world.getAudioEngine();
    auto session = world.getSession();
    
    Node node (Tags::graph);
    node.setProperty (Tags::name, "Graph " + String(session->getNumGraphs() + 1));
    addGraph (node);    
    
    findSibling<GuiController>()->stabilizeContent();
}

void EngineController::addGraph (const Node& newGraph)
{
    jassert(newGraph.isGraph());

    Node node       = newGraph.getValueTree().getParent().isValid() ? newGraph
                    : Node (newGraph.getValueTree().createCopy(), false);
    auto engine     = getWorld().getAudioEngine();
    auto session    = getWorld().getSession();
    String err      = node.isGraph() ? String() : "Not a graph";   

    if (err.isNotEmpty())
    {
        AlertWindow::showMessageBoxAsync (AlertWindow::InfoIcon, "Audio Engine", err);
        return;
    }

    if (auto* holder = graphs->add (new RootGraphHolder (node, getWorld())))
    {
        if (holder->attach (engine))
        {
            session->addGraph (node, true);
            setRootNode (node);
            holder->addMissingIONodes();
        }
        else
        {
            err = "Could not attach new graph to engine.";
        }
    }
    else
    {
        err = "Could not create new graph.";
    }
    
    if (err.isNotEmpty())
    {
        AlertWindow::showMessageBoxAsync (AlertWindow::InfoIcon, "Audio Engine", err);
    }

    findSibling<GuiController>()->stabilizeContent();
}

void EngineController::duplicateGraph (const Node& graph)
{
    Node duplicate (graph.getValueTree().createCopy());
    duplicate.setProperty (Tags::name, duplicate.getName().replace("(copy)","").trim() + String(" (copy)"));
    addGraph (duplicate);
}

void EngineController::duplicateGraph()
{
    auto& world  = getWorld();
    auto engine  = world.getAudioEngine();
    auto session = world.getSession();
    
    String err;
    const Node current (session->getCurrentGraph());
    duplicateGraph (current);
}

void EngineController::removeGraph (int index)
{
    auto& world  = getWorld();
    auto engine  = world.getAudioEngine();
    auto session = world.getSession();
    
    if (index < 0)
        index = session->getActiveGraphIndex();
    
    if (auto* holder = graphs->findByEngineIndex (index))
    {
        bool removeIt = false;
        if (holder->detach (engine))
        {
            ValueTree sgraphs = session->getValueTree().getChildWithName (Tags::graphs);
            sgraphs.removeChild (holder->model.getValueTree(), nullptr);
            removeIt = true;
        }
        else
        {
            DBG("[EL] could not detach root graph");
        }
        
        if (removeIt)
        {
            graphs->remove (holder);
            DBG("[EL] graph removed");
            ValueTree sgraphs = session->getValueTree().getChildWithName (Tags::graphs);
            
            if (index < 0 || index >= session->getNumGraphs())
                index = session->getNumGraphs() - 1;
            
            if (isPositiveAndBelow (index, session->getNumGraphs()))
            {
                sgraphs.setProperty (Tags::active, index, 0);
                setRootNode (session->getActiveGraph ());
            }
            else if (session->getNumGraphs() > 0)
            {
                DBG("[EL] failed to find appropriate index.");
                sgraphs.setProperty (Tags::active, 0, 0);
                setRootNode (session->getActiveGraph ());
            }
        }
    }
    else
    {
        DBG("[EL] could not find root graph index: " << index);
    }
    
    findSibling<GuiController>()->stabilizeContent();
}

void EngineController::connectChannels (const uint32 s, const int sc, const uint32 d, const int dc)
{
    if (auto* root = graphs->findActiveRootGraphController ())
    {
        auto src = root->getNodeForId (s);
        auto dst = root->getNodeForId (d);
        if (!src || !dst)
            return;
        addConnection (src->nodeId, src->getPortForChannel (PortType::Audio, sc, false),
                       dst->nodeId, dst->getPortForChannel (PortType::Audio, dc, true));
    }
}

void EngineController::removeConnection (const uint32 s, const uint32 sp, const uint32 d, const uint32 dp)
{
    if (auto* root = graphs->findActiveRootGraphController())
        root->removeConnection (s, sp, d, dp);
}

void EngineController::removeConnection (const uint32 s, const uint32 sp, const uint32 d, const uint32 dp, const Node& target)
{
    if (auto* controller = graphs->findGraphControllerFor (target))
        controller->removeConnection (s, sp, d, dp);
}

void EngineController::addNode (const Node& node, const Node& target,
                                const ConnectionBuilder& builder)
{
    if (auto* controller = graphs->findGraphControllerFor (target))
    {
        const uint32 nodeId = controller->addNode (node);
        if (GraphNodePtr ptr = controller->getNodeForId (nodeId))
        {
            builder.addConnections (*controller, nodeId);
        }
    }    
}

void EngineController::addNode (const Node& node)
{
    auto* root = graphs->findActiveRootGraphController();
    const uint32 nodeId = (root != nullptr) ? root->addNode (node) : KV_INVALID_NODE;
    if (KV_INVALID_NODE != nodeId)
    {
        const Node actual (root->getNodeModelForId (nodeId));
        if (getWorld().getSettings().showPluginWindowsWhenAdded())
            findSibling<GuiController>()->presentPluginWindow (actual);
    }
    else
    {
        AlertWindow::showMessageBox (AlertWindow::InfoIcon,
            "Audio Engine", String("Could not add node: ") + node.getName());
    }
}

void EngineController::addPlugin (const PluginDescription& desc, const bool verified, const float rx, const float ry)
{
    auto* root = graphs->findActiveRootGraphController();
    if (! root)
        return;

    OwnedArray<PluginDescription> plugs;
    if (! verified)
    {
        auto* format = getWorld().getPluginManager().getAudioPluginFormat (desc.pluginFormatName);
        jassert(format != nullptr);
        auto& list (getWorld().getPluginManager().availablePlugins());
        list.removeFromBlacklist (desc.fileOrIdentifier);
        if (list.scanAndAddFile (desc.fileOrIdentifier, false, plugs, *format)) {
            getWorld().getPluginManager().saveUserPlugins (getWorld().getSettings());
        }
    }
    else
    {
        plugs.add (new PluginDescription (desc));
    }
    
    if (plugs.size() > 0)
    {
        const auto nodeId = root->addFilter (plugs.getFirst(), rx, ry);
        if (KV_INVALID_NODE != nodeId)
        {
            const Node node (root->getNodeModelForId (nodeId));
            if (getWorld().getSettings().showPluginWindowsWhenAdded())
                findSibling<GuiController>()->presentPluginWindow (node);
        }
    }
    else
    {
        AlertWindow::showMessageBoxAsync (AlertWindow::NoIcon, "Add Plugin",
                                          String("Could not add ") + desc.name + " for an unknown reason");
    }
}

void EngineController::removeNode (const Node& node)
{
    const Node graph (node.getParentGraph());
    if (! graph.isGraph())
        return;
    
    if (auto* controller = graphs->findGraphControllerFor (graph))
    {
        if (auto* gui = findSibling<GuiController>())
            gui->closePluginWindowsFor (node, true);
        controller->removeFilter (node.getNodeId());
    }
}

void EngineController::removeNode (const uint32 nodeId)
{
    auto* root = graphs->findActiveRootGraphController();
    if (! root)
        return;
    if (auto* gui = findSibling<GuiController>())
        gui->closePluginWindowsFor (nodeId, true);
    root->removeFilter (nodeId);
}

void EngineController::disconnectNode (const Node& node)
{
    const auto graph (node.getParentGraph());
    if (auto* controller = graphs->findGraphControllerFor (graph))
        controller->disconnectFilter (node.getNodeId());
}

void EngineController::activate()
{
    Controller::activate();
    
    auto* app = dynamic_cast<AppController*> (getRoot());
    auto& globals (app->getWorld());
    auto& devices (globals.getDeviceManager());
    auto engine (globals.getAudioEngine());
    auto session (globals.getSession());
    engine->setSession (session);

    sessionReloaded();
    
    engine->activate();
    devices.addChangeListener (this);
}

void EngineController::deactivate()
{
    Controller::deactivate();
    auto& globals (getWorld());
    auto& devices (globals.getDeviceManager());
    auto engine   (globals.getAudioEngine());
    graphs->savePluginStates();
    graphs->detachAll();
    graphs->clear();
    engine->setSession (nullptr);
    devices.removeChangeListener (this);
}

void EngineController::clear()
{
    if (auto* root = graphs->findActiveRootGraphController())
        root->clear();
}

void EngineController::setRootNode (const Node& newRootNode)
{
    if (! newRootNode.isRootGraph())
    {
        jassertfalse; // needs to be a graph
        return;
    }
    
    auto* holder = graphs->findFor (newRootNode);
    if (! holder)
    {
        jassertfalse; // you should have a root graph registered before calling this.
        holder = graphs->add (new RootGraphHolder (newRootNode, getWorld()));
    }
    
    if (! holder)
    {
        DBG("[EL] failed to find root graph for node: " << newRootNode.getName());
        return;
    }
    
    auto engine   = getWorld().getAudioEngine();
    auto session  = getWorld().getSession();
    auto& devices = getWorld().getDeviceManager();
    
    if (! holder->attached())
        holder->attach (engine);
    const int index = holder->getRootGraph()->getEngineIndex();

#if 0
    // saving this for reference. graphs will need to be
    // explicitly de-activated by users to unload them
    // moving forward - MRF
    
    /* Unload the active graph if necessary */
    auto* active = graphs->findActiveInEngine();
    if (active && active != holder)
    {
        if (auto* gui = findSibling<GuiController>())
            gui->closeAllPluginWindows();
        
        if (! (bool) active->model.getProperty(Tags::persistent) && active->attached())
        {
            active->controller->savePluginStates();
            active->controller->unloadGraph();
            DBG("[EL] graph unloaded: " << active->model.getName());
        }
    }
#endif

    if (auto* proc = holder->getRootGraph())
    {
        proc->setMidiChannel ((int) newRootNode.getProperty (Tags::midiChannel, 0));
    }
    else
    {
        DBG("[EL] couldn't find graph processor for node.");
    }
    
    if (auto* r = holder->getController())
    {
        DBG("[EL] setting root: " << holder->model.getName());
        
        if (! (bool) newRootNode.getProperty (Tags::persistent, false) || !r->isLoaded())
        {
            DBG("[EL] loading...");
            r->getRootGraph().setPlayConfigFor (devices);
            r->setNodeModel (newRootNode);
        }
        
        engine->setCurrentGraph (index);
        
        if (auto* gui = findSibling<GuiController>())
            gui->showPluginWindowsFor (newRootNode);
    }
    else
    {
        DBG("[EL] no graph controller for node: " << newRootNode.getName());
    }
    
    engine->refreshSession();
}

void EngineController::updateRootGraphMidiChannel (const int index, const int midiChannel)
{
    auto engine   = getWorld().getAudioEngine();
    if (auto* g = engine->getGraph (index)) {
        g->setMidiChannel (midiChannel);
    }
}

void EngineController::addMissingIONodes()
{
    if (auto session = getWorld().getSession())
        if (auto* h = graphs->findFor (session->getActiveGraph()))
            h->addMissingIONodes();
}

void EngineController::changeListenerCallback (ChangeBroadcaster* cb)
{
    typedef GraphProcessor::AudioGraphIOProcessor IOP;
    auto& devices (getWorld().getDeviceManager());
    auto session = getWorld().getSession();
    auto* root = graphs->findActiveRootGraphController();
    
    if (cb == &devices && root != nullptr)
    {
        auto& processor (root->getRootGraph());
        if (auto* device = devices.getCurrentAudioDevice())
        {
            auto nodes = session->getActiveGraph().getValueTree().getChildWithName (Tags::nodes);
            processor.suspendProcessing (true);
            processor.setPlayConfigFor (devices);
            
            for (int i = nodes.getNumChildren(); --i >= 0;)
            {
                Node model (nodes.getChild (i), false);
                if (GraphNodePtr node = model.getGraphNode())
                    if (node && (node->isAudioIONode() || node->isMidiIONode()))
                        model.resetPorts();
            }
            
            root->syncArcsModel();
            processor.suspendProcessing (false);
        }
    }
}

void EngineController::addPlugin (const Node& graph, const PluginDescription& desc)
{
    if (! graph.isGraph())
        return;
    
    if (auto* controller = graphs->findGraphControllerFor (graph))
    {
        const Node node (addPlugin (*controller, desc));
    }
}

void EngineController::addPlugin (const Node& graph, const PluginDescription& desc,
                                  const ConnectionBuilder& builder)
{
    if (! graph.isGraph())
        return;
    
    if (auto* controller = graphs->findGraphControllerFor (graph))
    {
        const Node node (addPlugin (*controller, desc));
        builder.addConnections (*controller, node.getNodeId());
        
    }
}

void EngineController::sessionReloaded()
{
    graphs->clear();
    
    auto session = getWorld().getSession();
    auto engine  = getWorld().getAudioEngine();
    if (session->getNumGraphs() > 0)
    {
        for (int i = 0; i < session->getNumGraphs(); ++i)
        {
            if (auto* holder = graphs->add (new RootGraphHolder (session->getGraph (i), getWorld())))
            {
                holder->attach (engine);
                if (auto* const controller = holder->getController())
                {
                    // noop: saving this logical block
                }
            }
        }

        setRootNode (session->getCurrentGraph());
    }
}

Node EngineController::addPlugin (GraphController& c, const PluginDescription& desc)
{
    const auto nodeId = c.addFilter (&desc, 0.5f, 0.5f, 0);
    if (KV_INVALID_NODE != nodeId)
    {
        const Node node (c.getNodeModelForId (nodeId));
        if (getWorld().getSettings().showPluginWindowsWhenAdded())
            findSibling<GuiController>()->presentPluginWindow (node);
        return node;
    }

    return Node();
}

void EngineController::changeBusesLayout (const Node& n, const AudioProcessor::BusesLayout& layout)
{
    Node node  = n;
    Node graph = node.getParentGraph();
    GraphNodePtr ptr = node.getGraphNode();
    auto* controller = graphs->findGraphControllerFor (graph);
    if (! controller)
        return;
    
    if (AudioProcessor* proc = ptr ? ptr->getAudioProcessor () : nullptr)
    {
        GraphNodePtr ptr2 = graph.getGraphNode();
        if (auto* gp = dynamic_cast<GraphProcessor*> (ptr2->getAudioProcessor()))
        {
            if (proc->checkBusesLayoutSupported (layout))
            {   
                gp->suspendProcessing (true);
                gp->releaseResources();
                
                const bool wasNotSuspended = ! proc->isSuspended();
                proc->suspendProcessing (true);
                proc->releaseResources();
                const bool success = proc->setBusesLayoutWithoutEnabling (layout);
                if (wasNotSuspended)
                    proc->suspendProcessing (false);
                
        
                gp->prepareToPlay (gp->getSampleRate(), gp->getBlockSize());
                gp->suspendProcessing (false);

                controller->removeIllegalConnections();
                controller->syncArcsModel();
                node.resetPorts();

                bool b = false;
            }
        }
    }
}

}
