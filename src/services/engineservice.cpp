// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include "ElementApp.h"

#include <element/context.hpp>
#include <element/devices.hpp>
#include <element/graph.hpp>
#include <element/node.hpp>
#include <element/plugins.hpp>
#include <element/services.hpp>
#include <element/settings.hpp>

#include "engine/graphmanager.hpp"
#include "nodes/mididevice.hpp"
#include "engine/rootgraph.hpp"
#include <element/engine.hpp>
#include <element/ui.hpp>

namespace element {

namespace detail {
static void initializeRootGraphPorts (RootGraph* root, const Node& model)
{
    PortArray ins, outs;
    model.getPorts (ins, outs, PortType::Audio);
    root->setNumPorts (PortType::Audio, ins.size(), true, false);
    root->setNumPorts (PortType::Audio, outs.size(), false, false);

    ins.clearQuick();
    outs.clearQuick();
    model.getPorts (ins, outs, PortType::Midi);
    root->setNumPorts (PortType::Midi, ins.size(), true, false);
    root->setNumPorts (PortType::Midi, outs.size(), false, false);
}
} // namespace detail

struct RootGraphHolder
{
    RootGraphHolder (const Node& n, Context& world)
        : plugins (world.plugins()),
          devices (world.devices()),
          model (n)
    {
    }

    ~RootGraphHolder()
    {
        jassert (! attached());
        controller = nullptr;
        model.data().removeProperty (tags::object, 0);
        node = nullptr;
        model = Node();
    }

    bool attached() const { return node && controller; }

    /** This will create a root graph processor/controller and load it if not
        done already. Properties are set from the model, so make sure they are
        correct before calling this 
     */
    bool attach (AudioEnginePtr engine)
    {
        jassert (engine);
        if (! engine)
        {
            DBG ("[element] root graph attach: engine is nil");
            return false;
        }

        if (attached())
        {
            DBG ("[element] root graph attach: already attached");
            return true;
        }

        node = new RootGraph (engine->context());

        if (auto* root = getRootGraph())
        {
            const auto modeStr = model.getProperty (tags::renderMode, "single").toString().trim().toLowerCase();
            const auto mode = modeStr == "single" ? RootGraph::SingleGraph : RootGraph::Parallel;
            const auto channels = model.getMidiChannels();
            const auto program = (int) model.getProperty ("midiProgram", -1);

            // TODO: Uniform method for saving/restoring nodes with custom ports.
            detail::initializeRootGraphPorts (root, model);

            // root->setPlayConfigFor (devices);
            root->setRenderMode (mode);
            root->setMidiChannels (channels);
            root->setMidiProgram (program);

            if (engine->addGraph (root))
            {
                controller = std::make_unique<RootGraphManager> (*root, plugins);
                model.setProperty (tags::object, node.get());

                controller->setNodeModel (model);
            }
            else
            {
                std::clog << "[element] failed to set root graph\n";
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
            node = nullptr;
        }

        return wasRemoved;
    }

    RootGraphManager* getController() const { return controller.get(); }
    RootGraph* getRootGraph() const { return dynamic_cast<RootGraph*> (node ? node.get() : nullptr); }

    bool hasController() const { return nullptr != controller; }

private:
    friend class EngineService;
    friend class EngineService::RootGraphs;
    PluginManager& plugins;
    DeviceManager& devices;
    std::unique_ptr<RootGraphManager> controller;
    Node model;
    ProcessorPtr node;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RootGraphHolder);
};

class EngineService::RootGraphs
{
public:
    RootGraphs (EngineService& e) : owner (e) {}
    ~RootGraphs() {}

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
        return nullptr;
    }

    /** Returns the active graph according to the engine */
    RootGraphHolder* findActiveInEngine() const
    {
        auto engine = owner.context().audio();
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
        if (auto session = owner.context().session())
            if (auto* h = findFor (session->getActiveGraph()))
                return h;
        return 0;
    }

    /** This returns a GraphManager for the provided node. The
        passed in node is expected to have type="graph" 
        
        NOTE: this is a recursive operation
     */
    GraphManager* findGraphManagerFor (const Node& graph)
    {
        for (const auto* h : graphs)
        {
            if (auto* m1 = h->controller.get())
                if (auto* m2 = m1->findGraphManagerForGraph (graph))
                    return m2;
        }

        return nullptr;
    }

    RootGraphManager* findActiveRootGraphManager() const
    {
        if (auto* h = findActive())
            return h->controller.get();
        return 0;
    }

    void attachAll()
    {
        engine = owner.context().audio();
        for (auto* g : graphs)
            g->attach (engine);
    }

    void detachAll()
    {
        engine = owner.context().audio();
        for (auto* g : graphs)
            g->detach (engine);
    }

    // remove the holder, this will also delete it!
    void remove (RootGraphHolder* g)
    {
        graphs.removeObject (g, true);
    }

    const OwnedArray<RootGraphHolder>& getGraphs() const { return graphs; }

private:
    EngineService& owner;
    SessionPtr session;
    AudioEnginePtr engine;
    OwnedArray<RootGraphHolder> graphs;
};

EngineService::EngineService()
    : Service()
{
    graphs = std::make_unique<RootGraphs> (*this);
}

EngineService::~EngineService()
{
    graphs = nullptr;
}

void EngineService::addConnection (const uint32 s, const uint32 sp, const uint32 d, const uint32 dp)
{
    if (auto session = context().session())
        if (auto* h = graphs->findFor (session->getCurrentGraph()))
            if (auto* c = h->getController())
                c->addConnection (s, sp, d, dp);
}

void EngineService::addConnection (const uint32 s, const uint32 sp, const uint32 d, const uint32 dp, const Node& graph)
{
    if (auto* controller = graphs->findGraphManagerFor (graph))
        controller->addConnection (s, sp, d, dp);
}

void EngineService::addGraph()
{
    auto& world = context();
    auto engine = world.audio();
    auto session = world.session();

    Node node (Graph::create ("Graph " + String (session->getNumGraphs() + 1),
                              engine->getNumChannels (true),
                              engine->getNumChannels (false),
                              true,
                              true));

    addGraph (node, false);
}

void EngineService::addGraph (const Node& newGraph, bool makeActive)
{
    jassert (newGraph.isGraph());

    Node ret;
    Node node = newGraph.data().getParent().isValid() ? newGraph
                                                      : Node (newGraph.data().createCopy(), false);
    auto engine = context().audio();
    auto session = context().session();
    String err = node.isGraph() ? String() : "Not a graph";

    if (err.isNotEmpty())
    {
        AlertWindow::showMessageBoxAsync (AlertWindow::InfoIcon, "Audio Engine", err);
        return;
    }

    if (auto* holder = graphs->add (new RootGraphHolder (node, context())))
    {
        if (holder->attach (engine))
        {
            session->addGraph (node, makeActive);
            DBG ("[element] graph added: active: " << (int) makeActive);
            if (makeActive)
                setRootNode (node);
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
}

void EngineService::duplicateGraph (const Node& graph)
{
    Node duplicate (graph.data().createCopy());
    duplicate.savePluginState(); // need objects present to update processor states
    Node::sanitizeRuntimeProperties (duplicate.data());
    // reset UUIDs to avoid compilcations with undoable actions
    duplicate.forEach ([] (const ValueTree& tree) {
        if (! tree.hasType (types::Node))
            return;
        auto nodeRef = tree;
        nodeRef.setProperty (tags::uuid, Uuid().toString(), nullptr);
    });

    duplicate.setProperty (tags::name, duplicate.getName().replace ("(copy)", "").trim() + String (" (copy)"));
    addGraph (duplicate, true);
}

void EngineService::duplicateGraph()
{
    auto& world = context();
    auto engine = world.audio();
    auto session = world.session();
    const Node current (session->getCurrentGraph());
    duplicateGraph (current);
}

void EngineService::removeGraph (int index)
{
    auto& world = context();
    auto engine = world.audio();
    auto session = world.session();

    if (index < 0)
        index = session->getActiveGraphIndex();

    const auto toRemove = session->getGraph (index);
    const auto active = session->getActiveGraph();
    const bool removedIsActive = toRemove == active;

    if (! toRemove.isValid())
    {
        DBG ("[element] cannot remove invalid graph");
        return;
    }

    if (auto* holder = graphs->findByEngineIndex (index))
    {
        bool removeIt = false;
        if (holder->detach (engine))
        {
            ValueTree sgraphs = session->data().getChildWithName (tags::graphs);
            sgraphs.removeChild (holder->model.data(), nullptr);
            removeIt = true;
        }
        else
        {
            DBG ("[element] could not detach root graph");
        }

        if (removeIt)
        {
            graphs->remove (holder);
            DBG ("[element] graph removed: index: " << index);
            ValueTree sgraphs = session->data().getChildWithName (tags::graphs);
            if (removedIsActive)
            {
                if (index < 0 || index >= session->getNumGraphs())
                    index = session->getNumGraphs() - 1;

                sgraphs.setProperty (tags::active, index, 0);
                const Node nextGraph = session->getCurrentGraph();

                if (nextGraph.isRootGraph())
                {
                    DBG ("[element] setting new graph: " << nextGraph.getName());
                    setRootNode (nextGraph);
                }
                else if (session->getNumGraphs() > 0)
                {
                    DBG ("[element] failed to find appropriate index.");
                    sgraphs.setProperty (tags::active, 0, nullptr);
                    setRootNode (session->getActiveGraph());
                }
            }
            else
            {
                index = std::max (0, sgraphs.indexOf (active.data()));
                sgraphs.setProperty (tags::active, index, nullptr);
                setRootNode (active);
            }
        }
    }
    else
    {
        DBG ("[element] could not find root graph index: " << index);
    }

    if (toRemove.isValid())
        sigNodeRemoved (toRemove);
    // FIXME: dont notify the UI top-down
    sibling<UI>()->stabilizeContent();
}

void EngineService::connectChannels (const Node& graph, const Node& src, const int sc, const Node& dst, const int dc)
{
    connectChannels (graph, src.getNodeId(), sc, dst.getNodeId(), dc);
}

void EngineService::connectChannels (const Node& graph, const uint32 s, const int sc, const uint32 d, const int dc)
{
    if (auto* root = graphs->findGraphManagerFor (graph))
    {
        auto src = root->getNodeForId (s);
        auto dst = root->getNodeForId (d);
        if (! src || ! dst)
            return;
        // clang-format off
        addConnection (src->nodeId, 
            src->getPortForChannel (PortType::Audio, sc, false), 
            dst->nodeId, 
            dst->getPortForChannel (PortType::Audio, dc, true));
        // clang-format on
    }
}

void EngineService::connectChannels (const uint32 s, const int sc, const uint32 d, const int dc)
{
    connectChannels (context().session()->getActiveGraph(), s, sc, d, dc);
}

void EngineService::connect (PortType type, const Node& src, int sc, const Node& dst, int dc, int nc)
{
    if (nc < 1)
        nc = 1;
    if (auto manager = graphs->findGraphManagerFor (src.getParentGraph()))
    {
        auto s = src.getObject();
        auto d = dst.getObject();
        while (s && d && --nc >= 0)
        {
            auto sp = s->getPortForChannel (type, sc, false);
            auto dp = d->getPortForChannel (type, dc, true);
            if (! manager->addConnection (src.getNodeId(), sp, dst.getNodeId(), dp))
            {
                String msg = "[element] connection failed: \n";

                msg << "Source: " << src.getName() << juce::newLine
                    << " ch. " << (int) sc << juce::newLine
                    << " port " << (int) sp << juce::newLine
                    << "Target: " << dst.getName() << juce::newLine
                    << " ch. " << (int) dc << juce::newLine
                    << " port " << (int) dp << juce::newLine;
                DBG (msg);
                break;
            };

            ++sc;
            ++dc;
        }
        manager->removeIllegalConnections();
        manager->syncArcsModel();
    }
}

void EngineService::removeConnection (const uint32 s, const uint32 sp, const uint32 d, const uint32 dp)
{
    if (auto* root = graphs->findActiveRootGraphManager())
        root->removeConnection (s, sp, d, dp);
}

void EngineService::removeConnection (const uint32 s, const uint32 sp, const uint32 d, const uint32 dp, const Node& target)
{
    if (auto* controller = graphs->findGraphManagerFor (target))
        controller->removeConnection (s, sp, d, dp);
}

Node EngineService::addNode (const Node& node, const Node& target, const ConnectionBuilder& builder)
{
    auto ref = node;
    if (EL_NODE_VERSION > node.version())
    {
        String error;
        const auto data = Node::migrate (node.data(), error);
        if (error.isEmpty() && data.isValid())
            ref = Node (data, true);
    }

    if (auto* controller = graphs->findGraphManagerFor (target))
    {
        const uint32 nodeId = controller->addNode (ref);
        ref = controller->getNodeModelForId (nodeId);
        if (ref.isValid())
        {
            builder.addConnections (*controller, nodeId);
            return ref;
        }
    }

    return Node();
}

Node EngineService::addNode (const String& ID, const String& format)
{
    juce::PluginDescription desc;
    desc.fileOrIdentifier = ID;
    desc.pluginFormatName = format;
    return addPlugin (desc, true, .5f, .5f, true);
}

void EngineService::addNode (const Node& _node)
{
    auto node = _node;
    if (EL_NODE_VERSION != node.version())
    {
        String error;
        auto data = Node::migrate (_node.data(), error);
        if (data.isValid() && error.isEmpty())
        {
            node = Node (data, false);
        }
        else
        {
            AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "Error adding node", error);
            return;
        }
    }

    auto* root = graphs->findActiveRootGraphManager();
    const uint32 nodeId = (root != nullptr) ? root->addNode (node) : EL_INVALID_NODE;
    if (EL_INVALID_NODE != nodeId)
    {
        const Node actual (root->getNodeModelForId (nodeId));
        if (context().settings().showPluginWindowsWhenAdded())
            sibling<GuiService>()->presentPluginWindow (actual);
    }
    else
    {
        AlertWindow::showMessageBox (AlertWindow::InfoIcon,
                                     "Audio Engine",
                                     String ("Could not add node: ") + node.getName());
    }
}

Node EngineService::addPlugin (const PluginDescription& desc, const bool verified, const float rx, const float ry, bool dontShowUI)
{
    auto* root = graphs->findActiveRootGraphManager();
    if (! root)
    {
        return {};
    }

    OwnedArray<PluginDescription> plugs;
    if (! verified)
    {
        auto* format = context().plugins().getAudioPluginFormat (desc.pluginFormatName);
        jassert (format != nullptr);
        auto& list (context().plugins().getKnownPlugins());
        list.removeFromBlacklist (desc.fileOrIdentifier);
        list.removeType (desc);
        if (list.scanAndAddFile (desc.fileOrIdentifier, false, plugs, *format))
        {
            context().plugins().saveUserPlugins (context().settings());
        }
    }
    else
    {
        plugs.add (new PluginDescription (desc));
    }

    Node node;
    if (plugs.size() > 0)
    {
        const auto nodeId = root->addNode (plugs.getFirst(), rx, ry);
        if (EL_INVALID_NODE != nodeId)
        {
            node = root->getNodeModelForId (nodeId);
            if (! dontShowUI && context().settings().showPluginWindowsWhenAdded())
                sibling<GuiService>()->presentPluginWindow (node);
        }
    }
    else
    {
        AlertWindow::showMessageBoxAsync (AlertWindow::NoIcon, "Add Plugin", String ("Could not add ") + desc.name + " for an unknown reason");
    }
    return node;
}

void EngineService::removeNode (const Node& node)
{
    const Node graph (node.getParentGraph());
    if (! graph.isGraph())
        return;

    auto* const gui = sibling<GuiService>();
    if (auto* manager = graphs->findGraphManagerFor (graph))
    {
        jassert (manager->contains (node.getNodeId()));
        gui->closePluginWindowsFor (node, true);
        if (gui->getSelectedNode() == node)
            gui->selectNode (Node());
        manager->removeNode (node.getNodeId());
        sigNodeRemoved (node);
    }

    if (auto gp = dynamic_cast<GraphNode*> (graph.getObject()))
    {
        const bool asyncNotify = false;
        // workaround: removal of a duplex node in reality has
        // cleared the graph's ports
        if (node.isAudioInputNode())
        {
            gp->setNumPorts (PortType::Audio, 0, true, asyncNotify);
        }
        else if (node.isAudioOutputNode())
        {
            gp->setNumPorts (PortType::Audio, 0, false, asyncNotify);
        }
        else if (node.isMidiInputNode())
        {
            gp->setNumPorts (PortType::Midi, 0, true, asyncNotify);
        }
        else if (node.isMidiOutputNode())
        {
            gp->setNumPorts (PortType::Midi, 0, false, asyncNotify);
        }
    }
}

void EngineService::removeNode (const Uuid& uuid)
{
    auto session = context().session();
    const auto node = session->findNodeById (uuid);
    if (node.isValid())
    {
        removeNode (node);
    }
    else
    {
        DBG ("[element] node not found: " << uuid.toString());
    }
}

void EngineService::removeNode (const uint32 nodeId)
{
    auto* root = graphs->findActiveRootGraphManager();
    if (! root)
        return;
    if (auto* gui = sibling<GuiService>())
        gui->closePluginWindowsFor (nodeId, true);
    root->removeNode (nodeId);
}

void EngineService::disconnectNode (const Node& node, const bool inputs, const bool outputs, const bool audio, const bool midi)
{
    const auto graph (node.getParentGraph());
    if (auto* controller = graphs->findGraphManagerFor (graph))
        controller->disconnectNode (node.getNodeId(), inputs, outputs, audio, midi);
}

void EngineService::activate()
{
    Service::activate();

    auto& globals (context());
    auto engine (globals.audio());
    auto session (globals.session());
    engine->setSession (session);
    engine->activate();

    sessionReloaded();
}

void EngineService::deactivate()
{
    Service::deactivate();
    auto& globals (context());
    auto engine (globals.audio());
    auto session (globals.session());

    if (auto* const gui = sibling<UI>())
    {
        // UI might not deactivate before the engine, so
        // close the windows here
        gui->closeAllPluginWindows();
    }

    session->saveGraphState();
    graphs->clear();

    engine->deactivate();
    engine->setSession (nullptr);
}

void EngineService::clear()
{
    graphs->clear();
}

void EngineService::setRootNode (const Node& newRootNode)
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
        holder = graphs->add (new RootGraphHolder (newRootNode, context()));
    }

    if (! holder)
    {
        DBG ("[element] failed to find root graph for node: " << newRootNode.getName());
        return;
    }

    auto engine = context().audio();
    auto session = context().session();
    auto& devices = context().devices();

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
        if (auto* gui = sibling<GuiService>())
            gui->closeAllPluginWindows();
        
        if (! (bool) active->model.getProperty(tags::persistent) && active->attached())
        {
            active->controller->savePluginStates();
            active->controller->unloadGraph();
            DBG("[element] graph unloaded: " << active->model.getName());
        }
    }
#endif

    if (auto* proc = holder->getRootGraph())
    {
        proc->setMidiChannels (newRootNode.getMidiChannels().get());
        proc->setVelocityCurveMode ((VelocityCurve::Mode) (int) newRootNode.getProperty (
            tags::velocityCurveMode, (int) VelocityCurve::Linear));

        // TODO: Uniform method for saving/restoring nodes with custom ports.
        detail::initializeRootGraphPorts (proc, newRootNode);
    }
    else
    {
        DBG ("[element] couldn't find graph processor for node.");
    }

    if (auto* const r = holder->getController())
    {
        if (! r->isLoaded())
        {
            r->getRootGraph().setPlayConfigFor (devices);
            r->setNodeModel (newRootNode);
        }

        engine->setCurrentGraph (index);
    }
    else
    {
        DBG ("[element] no graph controller for node: " << newRootNode.getName());
    }

    engine->refreshSession();
}

void EngineService::syncModels()
{
    for (auto* holder : graphs->getGraphs())
    {
        Node graph (holder->model);
        for (int i = 0; i < graph.getNumNodes(); ++i)
        {
            Node node (graph.getNode (i));
            if (! node.isIONode())
                continue;
            node.resetPorts();
        }

        if (auto* controller = holder->getController())
        {
            controller->syncArcsModel();
        }
    }
}

Node EngineService::addPlugin (const Node& graph, const PluginDescription& desc)
{
    if (! graph.isGraph())
        return {};

    Node node;
    if (auto* controller = graphs->findGraphManagerFor (graph))
        node = addPlugin (*controller, desc);

    return node;
}

Node EngineService::addPlugin (const Node& graph, const PluginDescription& desc, const ConnectionBuilder& builder, const bool verified)
{
    if (! graph.isGraph())
        return Node();

    auto& list (context().plugins().getKnownPlugins());
    OwnedArray<PluginDescription> plugs;

    if (! verified)
    {
        if (desc.pluginFormatName == "LV2")
        {
            if (auto lv2 = context().plugins().getProvider ("LV2"))
            {
                juce::ignoreUnused (lv2);
                list.removeFromBlacklist (desc.fileOrIdentifier);
                list.addType (desc);
            }
        }
        else
        {
            auto* format = context().plugins().getAudioPluginFormat (desc.pluginFormatName);
            jassert (format != nullptr);

            list.removeFromBlacklist (desc.fileOrIdentifier);

            if (list.scanAndAddFile (desc.fileOrIdentifier, false, plugs, *format))
            {
                context().plugins().saveUserPlugins (context().settings());
            }
        }
    }
    else
    {
        plugs.add (new PluginDescription (desc));
    }

    const PluginDescription descToLoad = (plugs.size() > 0) ? *plugs.getFirst() : desc;

    if (auto* controller = graphs->findGraphManagerFor (graph))
    {
        const Node node (addPlugin (*controller, descToLoad));
        if (node.isValid())
        {
            builder.addConnections (*controller, node.getNodeId());
            jassert (! node.getUuid().isNull());
        }
        return node;
    }

    return Node();
}

void EngineService::sessionReloaded()
{
    graphs->clear();

    auto session = context().session();
    auto engine = context().audio();

    if (session->getNumGraphs() > 0)
    {
        for (int i = 0; i < session->getNumGraphs(); ++i)
        {
            Node rootGraph (session->getGraph (i));
            if (auto* holder = graphs->add (new RootGraphHolder (rootGraph, context())))
            {
                if (! holder->attach (engine))
                {
                    std::clog << "[element] failed attaching root grapn: " << holder->model.getName() << std::endl;
                }

                if (auto* const controller = holder->getController())
                {
                    // noop: saving this logical block
                }
            }
        }

        const auto ag = session->getActiveGraph();
        setRootNode (ag);
    }

    if (session->getNumGraphs() != graphs->getGraphs().size())
    {
        std::clog << "[element] model and engine graph counts do not match\n";
    }
    else
    {
        DBG ("[element] session reloaded: " << session->getName());
    }
}

Node EngineService::addPlugin (GraphManager& c, const PluginDescription& desc)
{
    auto& plugins (context().plugins());
    const auto nodeId = c.addNode (&desc, 0.5f, 0.5f, 0);

    if (EL_INVALID_NODE != nodeId)
    {
        plugins.addToKnownPlugins (desc);

        const Node node (c.getNodeModelForId (nodeId));
        if (context().settings().showPluginWindowsWhenAdded())
            sibling<GuiService>()->presentPluginWindow (node);
        if (! node.isValid())
        {
            jassertfalse; // fatal, but continue
        }

        if (node.getUuid().isNull())
        {
            jassertfalse;
            ValueTree nodeData = node.data();
            nodeData.setProperty (tags::uuid, Uuid().toString(), 0);
        }
        return node;
    }

    return Node();
}

Node EngineService::addMidiDeviceNode (const MidiDeviceInfo& device, const bool isInput)
{
    ProcessorPtr ptr;
    Node graph;
    if (auto s = context().session())
        graph = s->getActiveGraph();
    if (auto* const root = graphs->findActiveRootGraphManager())
    {
        PluginDescription desc;
        desc.pluginFormatName = EL_NODE_FORMAT_NAME;
        desc.fileOrIdentifier = isInput ? EL_NODE_ID_MIDI_INPUT_DEVICE
                                        : EL_NODE_ID_MIDI_OUTPUT_DEVICE;
        ptr = root->getNodeForId (root->addNode (&desc, 0.5, 0.5));
    }

    MidiDeviceProcessor* const proc = (ptr == nullptr) ? nullptr
                                                       : dynamic_cast<MidiDeviceProcessor*> (ptr->getAudioProcessor());

    Node deviceNode;
    if (proc != nullptr)
    {
        proc->setDevice (device);

        for (int i = 0; i < graph.getNumNodes(); ++i)
        {
            auto node (graph.getNode (i));
            if (node.getObject() == ptr.get())
            {
                node.setProperty (tags::name, proc->getDeviceName());
                node.resetPorts();
                deviceNode = node;
                break;
            }
        }
    }
    else
    {
        // noop
    }

    return deviceNode;
}

void EngineService::changeBusesLayout (const Node& n, const AudioProcessor::BusesLayout& layout)
{
    Node node = n;
    Node graph = node.getParentGraph();
    ProcessorPtr ptr = node.getObject();
    auto* controller = graphs->findGraphManagerFor (graph);
    if (! controller)
        return;

    if (AudioProcessor* proc = ptr ? ptr->getAudioProcessor() : nullptr)
    {
        ProcessorPtr ptr2 = graph.getObject();
        if (auto* gp = dynamic_cast<GraphNode*> (ptr2.get()))
        {
            if (proc->checkBusesLayoutSupported (layout))
            {
                gp->suspendProcessing (true);
                gp->releaseResources();

                const bool wasNotSuspended = ! proc->isSuspended();
                proc->suspendProcessing (true);
                proc->releaseResources();
                proc->setBusesLayoutWithoutEnabling (layout);
                node.resetPorts();
                if (wasNotSuspended)
                    proc->suspendProcessing (false);

                gp->prepareToRender (gp->getSampleRate(), gp->getBlockSize());
                gp->suspendProcessing (false);

                controller->removeIllegalConnections();
                controller->syncArcsModel();

                sibling<GuiService>()->stabilizeViews();
            }
        }
    }
}

void EngineService::replace (const Node& node, const PluginDescription& desc)
{
    const auto graph (node.getParentGraph());
    if (! graph.isGraph())
        return;

    if (auto* ctl = graphs->findGraphManagerFor (graph))
    {
        double x = 0.0, y = 0.0;
        node.getPosition (x, y);
        const auto oldNodeId = node.getNodeId();
        const auto wasWindowOpen = (bool) node.getProperty ("windowVisible");
        const auto nodeId = ctl->addNode (&desc, x, y);
        if (nodeId != EL_INVALID_NODE)
        {
            ProcessorPtr newptr = ctl->getNodeForId (nodeId);
            const ProcessorPtr oldptr = node.getObject();
            jassert (newptr && oldptr);
            // attempt to retain connections from the replaced node
            for (int i = ctl->getNumConnections(); --i >= 0;)
            {
                const auto* arc = ctl->getConnection (i);
                if (oldNodeId == arc->sourceNode)
                {
                    ctl->addConnection (
                        nodeId,
                        newptr->getPortForChannel (
                            oldptr->getPortType (arc->sourcePort),
                            oldptr->getChannelPort (arc->sourcePort),
                            oldptr->isPortInput (arc->sourcePort)),
                        arc->destNode,
                        arc->destPort);
                }
                else if (oldNodeId == arc->destNode)
                {
                    ctl->addConnection (
                        arc->sourceNode,
                        arc->sourcePort,
                        nodeId,
                        newptr->getPortForChannel (
                            oldptr->getPortType (arc->destPort),
                            oldptr->getChannelPort (arc->destPort),
                            oldptr->isPortInput (arc->destPort)));
                }
            }

            auto newNode (ctl->getNodeModelForId (nodeId));
            newNode.setPosition (x, y); // TODO: GraphManager should handle these
            newNode.setProperty ("windowX", (int) node.getProperty ("windowX"))
                .setProperty ("windowY", (int) node.getProperty ("windowY"));

            removeNode (node);
            if (wasWindowOpen)
                sibling<GuiService>()->presentPluginWindow (newNode);
        }
    }

    sibling<GuiService>()->stabilizeViews();
}

} // namespace element
