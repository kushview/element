/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <element/context.hpp>
#include <element/pluginmanager.hpp>
#include <element/settings.hpp>
#include <element/services.hpp>

#include "services/deviceservice.hpp"
#include "services/engineservice.hpp"
#include <element/services/guiservice.hpp>
#include "services/mappingservice.hpp"
#include "services/oscservice.hpp"
#include "services/sessionservice.hpp"
#include "services/presetservice.hpp"

#include "session/commandmanager.hpp"
#include "session/presetmanager.hpp"

#include "engine/graphmanager.hpp"
#include "gui/MainWindow.h"
#include "gui/GuiCommon.h"

#include "commands.hpp"
#include "messages.hpp"
#include "version.hpp"

namespace element {
using namespace juce;

class Services::Impl
{
public:
    Impl (Services& sm, Context& g, RunMode m)
        : owner (sm), world (g), runMode (m) {}
    void activate()
    {
        if (activated)
            return;

        if (! initialized)
        {
            lastExportedGraph = DataPath::defaultGraphDir();
            auto& commands = owner.context().getCommandManager();
            commands.registerAllCommandsForTarget (&owner);
            commands.registerAllCommandsForTarget (owner.find<GuiService>());
            commands.setFirstCommandTarget (&owner);
            initialized = true;
        }

        // migrate global node midi programs.
        auto progsdir = DataPath::defaultGlobalMidiProgramsDir();
        auto olddir = DataPath::applicationDataDir().getChildFile ("NodeMidiPrograms");
        if (! progsdir.exists() && olddir.exists())
        {
            progsdir.getParentDirectory().createDirectory();
            olddir.copyDirectoryTo (progsdir);
        }

        // restore recents
        const auto recentList = DataPath::applicationDataDir().getChildFile ("RecentFiles.txt");
        if (recentList.existsAsFile())
        {
            FileInputStream stream (recentList);
            recentFiles.restoreFromString (stream.readEntireStreamAsString());
        }

        for (auto* s : services)
            s->activate();

        activated = true;
    }

    void deactivate()
    {
        const auto recentList = DataPath::applicationDataDir().getChildFile ("RecentFiles.txt");
        if (! recentList.existsAsFile())
            recentList.create();
        if (recentList.exists())
            recentList.replaceWithText (recentFiles.toString(), false, false);

        for (auto* s : services)
            s->deactivate();

        activated = false;
    }

private:
    friend class Services;

    Services& owner;
    bool initialized { false };
    bool activated = false;
    juce::OwnedArray<Service> services;
    juce::File lastSavedFile;
    juce::File lastExportedGraph;
    Context& world;
    juce::RecentlyOpenedFilesList recentFiles;
    juce::UndoManager undo;
    RunMode runMode;
};

Services& Service::services() const
{
    jassert (owner != nullptr); // if you hit this then you're probably calling
        // services() before controller initialization
    return *owner;
}

Context& Service::context() { return services().context(); }
Settings& Service::getSettings() { return context().getSettings(); }
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

void Services::activate() { impl->activate(); }
void Services::deactivate() { impl->deactivate(); }

juce::RecentlyOpenedFilesList& Services::getRecentlyOpenedFilesList() { return impl->recentFiles; }
void Services::addRecentFile (const juce::File& file) { impl->recentFiles.addFile (file); }

RunMode Services::getRunMode() const { return impl->runMode; }
Context& Services::context() { return impl->world; }
juce::UndoManager& Services::getUndoManager() { return impl->undo; }

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

        if (context().getSettings().openLastUsedSession())
        {
            const auto lastSession = context().getSettings().getUserSettings()->getValue (Settings::lastSessionKey);
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
        auto* const props = context().getSettings().getUserSettings();

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

    auto& undo = impl->undo;
    auto& services = impl->services;
    auto& recentFiles = impl->recentFiles;

    if (const auto* message = dynamic_cast<const AppMessage*> (&msg))
    {
        OwnedArray<UndoableAction> actions;
        message->createActions (*this, actions);
        if (! actions.isEmpty())
        {
            undo.beginNewTransaction();
            for (auto* action : actions)
                undo.perform (action);
            actions.clearQuick (false);
            gui->stabilizeViews();
            return;
        }

        for (auto* const child : services)
        {
            if (auto* const acc = dynamic_cast<Service*> (child))
                handled = acc->handleMessage (*message);
            if (handled)
                break;
        }

        if (handled)
            return;
    }

    handled = true; // final else condition will set false
    if (const auto* lpm = dynamic_cast<const LoadPluginMessage*> (&msg))
    {
        ec->addPlugin (lpm->description, lpm->verified, lpm->relativeX, lpm->relativeY);
    }
    else if (const auto* dnm = dynamic_cast<const DuplicateNodeMessage*> (&msg))
    {
        Node node = dnm->node;
        ValueTree parent (node.getValueTree().getParent());
        if (parent.hasType (Tags::nodes))
            parent = parent.getParent();
        jassert (parent.hasType (Tags::node));

        const Node graph (parent, false);
        node.savePluginState();
        Node newNode (node.getValueTree().createCopy(), false);

        if (newNode.isValid() && graph.isValid())
        {
            newNode = Node (Node::resetIds (newNode.getValueTree()), false);
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
            AlertWindow alert ("Add Preset", "Enter preset name", AlertWindow::NoIcon, 0);
            alert.addTextEditor ("name", aps->node.getName());
            alert.addButton ("Save", 1, KeyPress (KeyPress::returnKey));
            alert.addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
            canceled = 0 == alert.runModalLoop();
            name = alert.getTextEditorContents ("name");
        }

        if (! canceled)
        {
            presets->add (node, name);
            node.setProperty (Tags::name, name);
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
            recentFiles.addFile (anm->sourceFile);
    }
    else if (const auto* cbm = dynamic_cast<const ChangeBusesLayout*> (&msg))
    {
        ec->changeBusesLayout (cbm->node, cbm->layout);
    }
    else if (const auto* osm = dynamic_cast<const OpenSessionMessage*> (&msg))
    {
        sess->openFile (osm->file);
        recentFiles.addFile (osm->file);
    }
    else if (const auto* mdm = dynamic_cast<const AddMidiDeviceMessage*> (&msg))
    {
        ec->addMidiDeviceNode (mdm->device, mdm->inputDevice);
    }
    else if (const auto* removeControllerDeviceMessage = dynamic_cast<const RemoveControllerDeviceMessage*> (&msg))
    {
        const auto device = removeControllerDeviceMessage->device;
        devs->remove (device);
    }
    else if (const auto* addControllerDeviceMessage = dynamic_cast<const AddControllerDeviceMessage*> (&msg))
    {
        const auto device = addControllerDeviceMessage->device;
        const auto file = addControllerDeviceMessage->file;
        if (file.existsAsFile())
        {
            devs->add (file);
        }
        else if (device.getValueTree().isValid())
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
    else if (const auto* refreshControllerDevice = dynamic_cast<const RefreshControllerDeviceMessage*> (&msg))
    {
        const auto device = refreshControllerDevice->device;
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
        if (graph.isValid() && node.isValid() && graph.getNodesValueTree() == node.getValueTree().getParent())
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

ApplicationCommandTarget* Services::getNextCommandTarget()
{
    return find<GuiService>();
}

void Services::getAllCommands (Array<CommandID>& cids)
{
    cids.addArray ({
        Commands::mediaNew,
        Commands::mediaOpen,
        Commands::mediaSave,
        Commands::mediaSaveAs,

        Commands::signIn,
        Commands::signOut,
        Commands::sessionNew,
        Commands::sessionSave,
        Commands::sessionSaveAs,
        Commands::sessionOpen,
        Commands::sessionAddGraph,
        Commands::sessionDuplicateGraph,
        Commands::sessionDeleteGraph,
        Commands::sessionInsertPlugin,

        Commands::importGraph,
        Commands::exportGraph,
        Commands::panic,
        Commands::checkNewerVersion,
        Commands::transportPlay,
        Commands::graphNew,
        Commands::graphOpen,
        Commands::graphSave,
        Commands::graphSaveAs,
        Commands::importSession,
        Commands::recentsClear,
    });
    cids.addArray ({ Commands::copy, Commands::paste, Commands::undo, Commands::redo });
}

void Services::getCommandInfo (CommandID commandID, ApplicationCommandInfo& result)
{
    find<GuiService>()->getCommandInfo (commandID, result);
    // for (auto* const child : getChildren())
    //     if (auto* const appChild = dynamic_cast<Controller*> (child))
    //         appChild->getCommandInfo (commandID, result);
}

bool Services::perform (const InvocationInfo& info)
{
    auto& undo = impl->undo;
    bool res = true;

    switch (info.commandID)
    {
        case Commands::undo: {
            if (undo.canUndo())
                undo.undo();
            if (auto* cc = find<GuiService>()->getContentComponent())
                cc->stabilizeViews();
            find<GuiService>()->refreshMainMenu();
        }
        break;

        case Commands::redo: {
            if (undo.canRedo())
                undo.redo();
            if (auto* cc = find<GuiService>()->getContentComponent())
                cc->stabilizeViews();
            find<GuiService>()->refreshMainMenu();
        }
        break;

        case Commands::sessionOpen: {
            FileChooser chooser ("Open Session", impl->lastSavedFile, "*.els", true, false);
            if (chooser.browseForFileToOpen())
            {
                find<SessionService>()->openFile (chooser.getResult());
                impl->recentFiles.addFile (chooser.getResult());
            }
        }
        break;

        case Commands::sessionNew:
            find<SessionService>()->newSession();
            break;
        case Commands::sessionSave:
            find<SessionService>()->saveSession (false);
            break;
        case Commands::sessionSaveAs:
            find<SessionService>()->saveSession (true);
            break;
        case Commands::sessionClose:
            find<SessionService>()->closeSession();
            break;
        case Commands::sessionAddGraph:
            find<EngineService>()->addGraph();
            break;
        case Commands::sessionDuplicateGraph:
            find<EngineService>()->duplicateGraph();
            break;
        case Commands::sessionDeleteGraph:
            find<EngineService>()->removeGraph();
            break;

        case Commands::transportPlay:
            context().audio()->togglePlayPause();
            break;

        case Commands::importGraph: {
            FileChooser chooser ("Import Graph", impl->lastExportedGraph, "*.elg");
            if (chooser.browseForFileToOpen())
                find<SessionService>()->importGraph (chooser.getResult());
        }
        break;

        case Commands::exportGraph: {
            auto session = context().session();
            auto node = session->getCurrentGraph();
            node.savePluginState();

            if (! impl->lastExportedGraph.isDirectory())
                impl->lastExportedGraph = impl->lastExportedGraph.getParentDirectory();
            if (impl->lastExportedGraph.isDirectory())
            {
                impl->lastExportedGraph = impl->lastExportedGraph.getChildFile (node.getName()).withFileExtension ("elg");
                impl->lastExportedGraph = impl->lastExportedGraph.getNonexistentSibling();
            }

            {
                FileChooser chooser ("Export Graph", impl->lastExportedGraph, "*.elg");
                if (chooser.browseForFileToSave (true))
                    find<SessionService>()->exportGraph (node, chooser.getResult());
                if (auto* gui = find<GuiService>())
                    gui->stabilizeContent();
            }
        }
        break;

        case Commands::panic: {
            auto e = context().audio();
            for (int c = 1; c <= 16; ++c)
            {
                auto msg = MidiMessage::allNotesOff (c);
                msg.setTimeStamp (Time::getMillisecondCounterHiRes());
                e->addMidiMessage (msg);
                msg = MidiMessage::allSoundOff (c);
                msg.setTimeStamp (Time::getMillisecondCounterHiRes());
                e->addMidiMessage (msg);
            }
        }
        break;

        case Commands::mediaNew:
        case Commands::mediaSave:
        case Commands::mediaSaveAs:
            break;

        case Commands::signIn: {
        }
        break;

        case Commands::signOut: {
            // noop
        }
        break;

        case Commands::checkNewerVersion:
            find<GuiService>()->checkUpdates();
            break;

        case Commands::recentsClear: {
            impl->recentFiles.clear();
            find<GuiService>()->refreshMainMenu();
        }
        break;

        default:
            res = false;
            break;
    }

    return res;
}

} // namespace element
