// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/context.hpp>
#include <element/plugins.hpp>
#include <element/settings.hpp>
#include <element/services.hpp>

#include <element/engine.hpp>
#include <element/ui.hpp>

#include "engine/graphmanager.hpp"
#include "presetmanager.hpp"

#include "services/deviceservice.hpp"
#include "services/mappingservice.hpp"
#include "services/oscservice.hpp"
#include "services/sessionservice.hpp"
#include "services/presetservice.hpp"
#include "messages.hpp"

namespace element {
using namespace juce;

class Services::Impl
{
public:
    Impl (Services& sm, Context& g, RunMode m)
        : owner (sm), world (g), runMode (m) {}

    void initialize()
    {
        if (initialized)
            return;
        for (auto* srv : services)
            srv->initialize();
    }

    void activate()
    {
        if (activated)
            return;

        if (! initialized)
        {
            lastExportedGraph = DataPath::defaultGraphDir();
            initialized = true;

            // migrate global node midi programs.
            auto progsdir = DataPath::defaultGlobalMidiProgramsDir();
            auto olddir = DataPath::applicationDataDir().getChildFile ("NodeMidiPrograms");
            if (! progsdir.exists() && olddir.exists())
            {
                progsdir.getParentDirectory().createDirectory();
                olddir.copyDirectoryTo (progsdir);
            }
        }

        for (auto* s : services)
            s->activate();

        activated = true;
    }

    void deactivate()
    {
        for (auto* s : services)
            s->deactivate();

        activated = false;
    }

private:
    friend class Services;

    [[maybe_unused]] Services& owner;
    bool initialized { false };
    bool activated = false;
    juce::OwnedArray<Service> services;
    juce::File lastSavedFile;
    juce::File lastExportedGraph;
    Context& world;
    RunMode runMode;
};

Services& Service::services() const
{
    jassert (owner != nullptr); // if you hit this then you're probably calling
    // services() before controller initialization
    return *owner;
}

Context& Service::context() { return services().context(); }
Settings& Service::settings() { return context().settings(); }
RunMode Service::getRunMode() const { return services().getRunMode(); }

Services::Services (Context& g, RunMode m)
{
    impl = std::make_unique<Impl> (*this, g, m);
    add (new GuiService (g, *this));
    add (new DeviceService());
    add (new EngineService());
    add (new MappingService());
    add (new PresetService());
    add (new SessionService());
    add (new OSCService());
}

Services::~Services()
{
    impl.reset();
}

void Services::initialize() { impl->initialize(); }
void Services::activate() { impl->activate(); }
void Services::deactivate() { impl->deactivate(); }

RunMode Services::getRunMode() const { return impl->runMode; }
Context& Services::context() { return impl->world; }

Service** Services::begin() noexcept { return impl->services.begin(); }
Service* const* Services::begin() const noexcept { return impl->services.begin(); }
Service** Services::end() noexcept { return impl->services.end(); }
Service* const* Services::end() const noexcept { return impl->services.end(); }

void Services::add (Service* service)
{
    service->owner = this;
    impl->services.add (service);
}

void Services::launch()
{
    activate();
    if (auto* gui = find<GuiService>())
        gui->run();
}

void Services::shutdown()
{
    for (auto* s : impl->services)
        s->shutdown();
}

void Services::saveSettings()
{
    for (auto* s : impl->services)
        s->saveSettings();
}

void Services::run()
{
    activate();

    // need content component parented for the following init routines
    // TODO: better controlled startup procedure
    if (auto* gui = find<GuiService>())
        gui->run();

    auto session = context().session();
    Session::ScopedFrozenLock freeze (*session);

    if (auto* sc = find<SessionService>())
    {
        bool loadDefault = true;

        if (context().settings().openLastUsedSession())
        {
            const auto lastSession = context().settings().getUserSettings()->getValue (Settings::lastSessionKey);
            if (File::isAbsolutePath (lastSession) && File (lastSession).existsAsFile())
            {
                sc->openFile (File (lastSession));
                loadDefault = false;
            }
        }

        if (loadDefault)
            sc->openDefaultSession();
    }

    if (auto* gui = find<GuiService>())
    {
        gui->stabilizeContent();
        const Node graph (session->getCurrentGraph());
        auto* const props = context().settings().getUserSettings();

        if (graph.isValid())
        {
            // don't show plugin windows on load if the UI was hidden
            if (props->getBoolValue ("mainWindowVisible", true))
                gui->showPluginWindowsFor (graph);
        }
    }
}

void Services::handleMessage (const Message& msg)
{
    auto* ec = find<EngineService>();
    auto* gui = find<GuiService>();
    auto* sess = find<SessionService>();
    auto* devs = find<DeviceService>();
    auto* maps = find<MappingService>();
    auto* presets = find<PresetService>();
    jassert (ec && gui && sess && devs && maps && presets);

    bool handled = false; // final else condition will set false
    auto& services = impl->services;

    if (const auto* message = dynamic_cast<const AppMessage*> (&msg))
    {
        for (auto* const child : services)
        {
            handled = child->handleMessage (*message);
            if (handled)
                break;
        }

        if (handled)
        {
            gui->refreshMainMenu();
            return;
        }
    }

    handled = true; // final else condition will set false
    if (const auto* lpm = dynamic_cast<const LoadPluginMessage*> (&msg))
    {
        ec->addPlugin (lpm->description, lpm->verified, lpm->relativeX, lpm->relativeY);
    }
    else if (const auto* dnm = dynamic_cast<const DuplicateNodeMessage*> (&msg))
    {
        Node node = dnm->node;
        ValueTree parent (node.data().getParent());
        if (parent.hasType (tags::nodes))
            parent = parent.getParent();
        jassert (parent.hasType (types::Node));

        const Node graph (parent, false);
        node.savePluginState();
        Node newNode (node.data().createCopy(), false);
        Node::sanitizeProperties (newNode.data(), true);
        if (newNode.version() < EL_NODE_VERSION)
        {
            std::clog << "[element] Dupliate node out of date?" << std::endl;
            // TODO: migrate
            newNode.setProperty (tags::version, (int) EL_NODE_VERSION);
        }

        if (newNode.isValid() && graph.isValid())
        {
            newNode = Node (Node::resetIds (newNode.data()), false);
            ConnectionBuilder dummy;
            ec->addNode (newNode, graph, dummy);
        }
    }
    else if (const auto* dnm2 = dynamic_cast<const DisconnectNodeMessage*> (&msg))
    {
        ec->disconnectNode (dnm2->node, dnm2->inputs, dnm2->outputs, dnm2->audio, dnm2->midi);
    }
    else if (const auto* aps = dynamic_cast<const AddPresetMessage*> (&msg))
    {
        String name = aps->name;
        Node node = aps->node;
        bool canceled = false;

        if (name.isEmpty())
        {
            AlertWindow alert (TRANS ("Save Node"), TRANS ("Enter a node name"), AlertWindow::NoIcon, 0);
            alert.addTextEditor ("name", aps->node.getName());
            alert.addButton ("Save", 1, KeyPress (KeyPress::returnKey));
            alert.addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
            canceled = 0 == alert.runModalLoop();
            name = alert.getTextEditorContents ("name");
        }

        if (! canceled)
        {
            presets->add (node, name);
            node.setProperty (tags::name, name);
        }
    }
    else if (const auto* sdnm = dynamic_cast<const SaveDefaultNodeMessage*> (&msg))
    {
        auto node = sdnm->node;
        node.savePluginState();
        context().plugins().saveDefaultNode (node);
    }
    else if (const auto* anm = dynamic_cast<const AddNodeMessage*> (&msg))
    {
        if (anm->target.isValid())
            ec->addNode (anm->node, anm->target, anm->builder);
        else
            ec->addNode (anm->node);

        if (anm->sourceFile.existsAsFile() && anm->sourceFile.hasFileExtension (".elg"))
            find<UI>()->recentFiles().addFile (anm->sourceFile);
    }
    else if (const auto* cbm = dynamic_cast<const ChangeBusesLayout*> (&msg))
    {
        ec->changeBusesLayout (cbm->node, cbm->layout);
        if (cbm->onFinished)
            cbm->onFinished();
    }
    else if (const auto* osm = dynamic_cast<const OpenSessionMessage*> (&msg))
    {
        sess->openFile (osm->file);
        find<UI>()->recentFiles().addFile (osm->file);
    }
    else if (const auto* mdm = dynamic_cast<const AddMidiDeviceMessage*> (&msg))
    {
        ec->addMidiDeviceNode (mdm->device, mdm->inputDevice);
    }
    else if (const auto* removeControllerMessage = dynamic_cast<const RemoveControllerMessage*> (&msg))
    {
        const auto device = removeControllerMessage->device;
        devs->remove (device);
    }
    else if (const auto* addControllerMessage = dynamic_cast<const AddControllerMessage*> (&msg))
    {
        const auto device = addControllerMessage->device;
        const auto file = addControllerMessage->file;
        if (file.existsAsFile())
        {
            devs->add (file);
        }
        else if (device.data().isValid())
        {
            devs->add (device);
        }
        else
        {
            DBG ("[element] add controller device not valid");
        }
    }
    else if (const auto* removeControlMessage = dynamic_cast<const RemoveControlMessage*> (&msg))
    {
        const auto device = removeControlMessage->device;
        const auto control = removeControlMessage->control;
        devs->remove (device, control);
    }
    else if (const auto* addControlMessage = dynamic_cast<const AddControlMessage*> (&msg))
    {
        const auto device (addControlMessage->device);
        const auto control (addControlMessage->control);
        devs->add (device, control);
    }
    else if (const auto* refreshController = dynamic_cast<const RefreshControllerMessage*> (&msg))
    {
        const auto device = refreshController->device;
        devs->refresh (device);
    }
    else if (const auto* removeMapMessage = dynamic_cast<const RemoveControllerMapMessage*> (&msg))
    {
        const auto controllerMap = removeMapMessage->controllerMap;
        maps->remove (controllerMap);
        gui->stabilizeViews();
    }
    else if (const auto* replaceNodeMessage = dynamic_cast<const ReplaceNodeMessage*> (&msg))
    {
        const auto graph = replaceNodeMessage->graph;
        const auto node = replaceNodeMessage->node;
        const auto desc (replaceNodeMessage->description);
        if (graph.isValid() && node.isValid() && graph.getNodesValueTree() == node.data().getParent())
        {
            ec->replace (node, desc);
        }
    }
    else
    {
        handled = false;
    }

    if (! handled)
    {
        DBG ("[element] unhandled Message received");
    }
}

} // namespace element
