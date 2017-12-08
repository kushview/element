
#include "ElementApp.h"
#include "controllers/AppController.h"
#include "controllers/GuiController.h"
#include "controllers/GraphController.h"
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
    
    RootGraphController* findActiveRootGraphController() const
    {
        if (auto session = owner.getWorld().getSession())
            if (auto* h = findFor (session->getActiveGraph()))
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

void EngineController::addGraph()
{
    auto& world  = getWorld();
    auto engine  = world.getAudioEngine();
    auto session = world.getSession();
    
    String err;
    Node node (Tags::graph);
    node.setProperty (Tags::name, "Graph " + String(session->getNumGraphs() + 1));
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

void EngineController::duplicateGraph()
{
    auto& world  = getWorld();
    auto engine  = world.getAudioEngine();
    auto session = world.getSession();
    
    String err;
    const Node current (session->getCurrentGraph());
    Node node (current.getValueTree().createCopy());
    node.setProperty (Tags::name, node.getName().replace("(copy)","").trim() + String(" (copy)"));
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
    
    if (err.isNotEmpty())
    {
        AlertWindow::showMessageBoxAsync (
            AlertWindow::InfoIcon, "Audio Engine", "Could not duplicate graph: " + current.getName());
    }
    
    findSibling<GuiController>()->stabilizeContent();
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

void EngineController::addNode (const Node& node)
{
    auto* root = graphs->findActiveRootGraphController();
    const uint32 nodeId = (root != nullptr) ? root->addNode (node) : KV_INVALID_NODE;
    if (KV_INVALID_NODE != nodeId)
    {
        const Node actual (root->getNodeModelForId (nodeId));
        findSibling<GuiController>()->presentPluginWindow (actual);
    }
    else
    {
        AlertWindow::showMessageBox (AlertWindow::InfoIcon,
            "Duplicate Node", String("Could not duplicate node: ") + node.getName());
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
        if (list.scanAndAddFile (desc.fileOrIdentifier, false, plugs, *format)) {
            getWorld().getPluginManager().saveUserPlugins (getWorld().getSettings());
        }
    }
    else
    {
        plugs.add (new PluginDescription (desc));
    }
    
    if (plugs.size() > 0)
        root->addFilter (plugs.getFirst(), rx, ry);
    else
        AlertWindow::showMessageBoxAsync (AlertWindow::NoIcon, "Add Plugin",
                                          String("Could not add ") + desc.name + " for an unknown reason");
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
    auto* root = graphs->findActiveRootGraphController();
    if (! root)
        return;
    root->disconnectFilter (node.getNodeId ());
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

    /* Unload the existing graph if necessary */
    if (auto* r = holder->getController())
    {
        if (auto* gui = findSibling<GuiController>())
            gui->closeAllPluginWindows();
        r->savePluginStates();
        r->unloadGraph();
    }

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
        DBG("[EL] setting root: " << newRootNode.getName());
        r->setNodeModel (newRootNode);
        if (auto* device = devices.getCurrentAudioDevice())
            r->getRootGraph().setPlayConfigFor (device);
        
        ValueTree nodes = newRootNode.getNodesValueTree();
        for (int i = nodes.getNumChildren(); --i >= 0;)
        {
            Node model (nodes.getChild (i), false);
            GraphNodePtr node = model.getGraphNode();
            if (node && node->isAudioIONode())
                model.resetPorts();
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
    auto* app = dynamic_cast<AppController*> (getRoot());
    auto& devices (app->getWorld().getDeviceManager());
   
    auto* root = graphs->findActiveRootGraphController();
    if (cb == (ChangeBroadcaster*) &devices && root != nullptr)
    {
        auto& processor (root->getRootGraph());
        if (auto* device = devices.getCurrentAudioDevice())
        {
            auto session = app->getWorld().getSession();
            auto nodes = session->getActiveGraph().getValueTree().getChildWithName(Tags::nodes);
            processor.suspendProcessing (true);
            processor.setPlayConfigFor (device);
            
            for (int i = nodes.getNumChildren(); --i >= 0;)
            {
                Node model (nodes.getChild(i), false);
                GraphNodePtr node = model.getGraphNode();
                if (node && node->isAudioIONode())
                {
                    (dynamic_cast<IOP*>(node->getProcessor()))->releaseResources();
                    (dynamic_cast<IOP*>(node->getProcessor()))->setParentGraph (&processor);
                    (dynamic_cast<IOP*>(node->getProcessor()))->prepareToPlay (device->getCurrentSampleRate(),
                                                                               device->getCurrentBufferSizeSamples());

                    model.resetPorts();
                }
            }
            
            root->syncArcsModel();
            processor.suspendProcessing (false);
        }
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
            if (auto* holder = graphs->add (new RootGraphHolder (session->getGraph (i), getWorld())))
                holder->attach (engine);
        
        setRootNode (session->getCurrentGraph());
    }
}
    
}
