
#include "controllers/AppController.h"
#include "controllers/DevicesController.h"
#include "controllers/EngineController.h"
#include "controllers/GuiController.h"
#include "controllers/GraphController.h"
#include "controllers/MappingController.h"
#include "controllers/SessionController.h"
#include "controllers/PresetsController.h"

#include "engine/GraphProcessor.h"
#include "engine/SubGraphProcessor.h"
#include "gui/UnlockForm.h"
#include "gui/GuiCommon.h"
#include "session/UnlockStatus.h"
#include "session/Presets.h"
#include "Commands.h"
#include "Globals.h"
#include "Messages.h"
#include "Settings.h"
#include "Version.h"

namespace Element {

Globals& AppController::Child::getWorld()
{
    auto* app = dynamic_cast<AppController*> (getRoot());
    jassert (app);
    return app->getWorld();
}

AppController::AppController (Globals& g)
    : world (g)
{
    lastExportedGraph = DataPath::defaultGraphDir();
    addChild (new GuiController (g, *this));
    addChild (new DevicesController ());
    addChild (new EngineController ());
    addChild (new SessionController ());
    addChild (new MappingController ());
    addChild (new PresetsController ());
    
    g.getCommandManager().registerAllCommandsForTarget (this);
    g.getCommandManager().setFirstCommandTarget (this);
}

AppController::~AppController()
{ 

}

void AppController::activate()
{
    const auto recentList = DataPath::applicationDataDir().getChildFile ("RecentFiles.txt");
    if (recentList.existsAsFile())
    {
        FileInputStream stream (recentList);
        recentFiles.restoreFromString (stream.readEntireStreamAsString());
    }

    Controller::activate();
}

void AppController::deactivate()
{
    const auto recentList = DataPath::applicationDataDir().getChildFile ("RecentFiles.txt");
    if (! recentList.existsAsFile())
        recentList.create();
    if (recentList.exists())
        recentList.replaceWithText (recentFiles.toString(), false, false);
    
    Controller::deactivate();
}

void AppController::run()
{
    auto* const sessCtl = findChild<SessionController>();
    auto* const devsCtl = findChild<DevicesController>();
    auto* const mapsCtl = findChild<MappingController>();
    auto* const presets = findChild<PresetsController>();
    assert(sessCtl && devsCtl && mapsCtl && presets);
    sessCtl->setChangesFrozen (true);

    activate();
    
    auto session = getWorld().getSession();

    if (auto* gui = findChild<GuiController>())
        gui->run();
    
    if (auto* sc = findChild<SessionController>())
    {
        bool loadDefault = true;

        if (world.getSettings().openLastUsedSession())
        {
            const auto lastSession = getWorld().getSettings().getUserSettings()->getValue ("lastSession");
            if (File::isAbsolutePath(lastSession) && File(lastSession).existsAsFile())
            {
                sc->openFile (File (lastSession));
                loadDefault = false;
            }
        }

        if (loadDefault)
            sc->openDefaultSession();
    }
    
    sessCtl->resetChanges();
    sessCtl->setChangesFrozen (false);
    devsCtl->refresh();
    presets->refresh();

    if (auto* gui = findChild<GuiController>())
    {
        const Node graph (session->getCurrentGraph());
        gui->stabilizeContent();
        if (graph.isValid())
            gui->showPluginWindowsFor (graph);
    }
}

void AppController::handleMessage (const Message& msg)
{
	auto* ec        = findChild<EngineController>();
    auto* gui       = findChild<GuiController>();
    auto* sess      = findChild<SessionController>();
    auto* devs      = findChild<DevicesController>();
    auto* maps      = findChild<MappingController>();
    auto* presets   = findChild<PresetsController>();
	jassert(ec && gui && sess && devs && maps && presets);

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
            return;
        }
    }

    bool handled = true; // final else condition will set false
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
            newNode.getValueTree().removeProperty (Tags::id, 0);
            ConnectionBuilder dummy;
            ec->addNode (newNode, graph, dummy);
        }
    }
    else if (const auto* dnm2 = dynamic_cast<const DisconnectNodeMessage*> (&msg))
    {
        ec->disconnectNode (dnm2->node, dnm2->inputs, dnm2->outputs);
    }
    else if (const auto* aps = dynamic_cast<const AddPresetMessage*> (&msg))
    {
        String name = aps->name;
        const Node node = aps->node;
        bool canceled = false;

        if (name.isEmpty ())
        {
            AlertWindow alert ("Add Preset", "Enter preset name", AlertWindow::NoIcon, 0);
            alert.addTextEditor ("name", aps->node.getName());
            alert.addButton ("Save", 1, KeyPress (KeyPress::returnKey));
            alert.addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
            canceled = 0 == alert.runModalLoop();
            name = alert.getTextEditorContents ("name");
        }

        if (! canceled)
            presets->add (node, name);
    }
    else if (const auto* anm = dynamic_cast<const AddNodeMessage*> (&msg))
    {
        if (anm->node.isGraph() && !getWorld().getUnlockStatus().isFullVersion())
        {
            Alert::showProductLockedAlert ("Nested graphs are not supported without a license");
        }

        else if (anm->target.isValid ())
            ec->addNode (anm->node, anm->target, anm->builder);
        else
            ec->addNode (anm->node);
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
        const auto file   = addControllerDeviceMessage->file;
        if (file.existsAsFile())
        {
            devs->add (file);
        }
        if (device.getValueTree().isValid())
        {
            devs->add (device);
        }
        else
        {
            DBG("[EL] add controller device not valid");
            DBG(device.getValueTree().toXmlString());
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
        gui->stabilizeContent();
    }
    else if (const auto* replaceNodeMessage = dynamic_cast<const ReplaceNodeMessage*> (&msg))
    {
        const auto graph = replaceNodeMessage->graph;
        const auto node  = replaceNodeMessage->node;
        const auto desc (replaceNodeMessage->description);
        if (graph.isValid() && node.isValid() && 
            graph.getNodesValueTree() == node.getValueTree().getParent())
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
        DBG("[EL] unhandled Message received");
    }
}

ApplicationCommandTarget* AppController::getNextCommandTarget()
{
    return findChild<GuiController>();
}

void AppController::getAllCommands (Array<CommandID>& cids)
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
        
        Commands::transportPlay
    });
    cids.addArray({ Commands::copy, Commands::paste, Commands::undo, Commands::redo });
}

void AppController::getCommandInfo (CommandID commandID, ApplicationCommandInfo& result)
{
    findChild<GuiController>()->getCommandInfo (commandID, result);
}

bool AppController::perform (const InvocationInfo& info)
{
    auto& status (world.getUnlockStatus());
	auto& settings(getGlobals().getSettings());
    bool res = true;
    switch (info.commandID)
    {
        case Commands::undo: {
            if (undo.canUndo())
                undo.undo();
            if (auto* cc = findChild<GuiController>()->getContentComponent())
                cc->stabilizeViews();
            findChild<GuiController>()->refreshMainMenu();

        } break;
        
        case Commands::redo:{
            if (undo.canRedo())
                undo.redo();
            if (auto* cc = findChild<GuiController>()->getContentComponent())
                cc->stabilizeViews();
            findChild<GuiController>()->refreshMainMenu();
        } break;

        case Commands::sessionOpen:
        {
            FileChooser chooser ("Open Session", lastSavedFile, "*.els", true, false);
            if (chooser.browseForFileToOpen())
            {
                findChild<SessionController>()->openFile (chooser.getResult());
                recentFiles.addFile (chooser.getResult());
            }

        } break;

        case Commands::sessionNew:
            findChild<SessionController>()->newSession();
            break;
        case Commands::sessionSave:
            findChild<SessionController>()->saveSession (false);
            break;
        case Commands::sessionSaveAs:
            findChild<SessionController>()->saveSession (true);
            break;
        case Commands::sessionClose:
            findChild<SessionController>()->closeSession();
            break;
        case Commands::sessionAddGraph:
            findChild<EngineController>()->addGraph();
            break;
        case Commands::sessionDuplicateGraph:
            findChild<EngineController>()->duplicateGraph();
            break;
        case Commands::sessionDeleteGraph:
            findChild<EngineController>()->removeGraph();
            break;
        
        case Commands::transportPlay:
            getWorld().getAudioEngine()->togglePlayPause();
            break;
            
        case Commands::importGraph:
        {
            if (world.getUnlockStatus().isFullVersion())
            {
                FileChooser chooser ("Import Graph", lastExportedGraph, "*.elg");
                if (chooser.browseForFileToOpen())
                    findChild<SessionController>()->importGraph (chooser.getResult());
            }
            else
            {
                Alert::showProductLockedAlert();
            }
            
        } break;
            
        case Commands::exportGraph:
        {
            auto session = getWorld().getSession();
            auto node = session->getCurrentGraph();
            node.savePluginState();
            
            if (!lastExportedGraph.isDirectory())
                lastExportedGraph = lastExportedGraph.getParentDirectory();
            if (lastExportedGraph.isDirectory())
            {
                lastExportedGraph = lastExportedGraph.getChildFile(node.getName()).withFileExtension ("elg");
                lastExportedGraph = lastExportedGraph.getNonexistentSibling();
            }
            if (world.getUnlockStatus().isFullVersion())
            {
                FileChooser chooser ("Export Graph", lastExportedGraph, "*.elg");
                if (chooser.browseForFileToSave (true))
                    findChild<SessionController>()->exportGraph (node, chooser.getResult());
                if (auto* gui = findChild<GuiController>())
                    gui->stabilizeContent();
            }
            else
            {
                Alert::showProductLockedAlert();
            }
        } break;

        case Commands::panic:
        {
            auto e = getWorld().getAudioEngine();
            for (int c = 1; c <= 16; ++c)
            {
                auto msg = MidiMessage::allNotesOff (c);
                msg.setTimeStamp (Time::getMillisecondCounterHiRes());
                e->addMidiMessage (msg);
                msg = MidiMessage::allSoundOff(c);
                msg.setTimeStamp (msg.getTimeStamp());
                e->addMidiMessage (msg);
            }
        }  break;
            
        case Commands::mediaNew:
        case Commands::mediaSave:
        case Commands::mediaSaveAs:
            break;
        
        case Commands::signIn:
        {
            auto* form = new UnlockForm (getWorld(), *findChild<GuiController>(),
                                        "Enter your license key.",
                                         false, false, true, true);
            DialogWindow::LaunchOptions opts;
            opts.content.setOwned (form);
            opts.resizable = false;
            opts.dialogTitle = "License Manager";
            opts.runModal();
        } break;
        
        case Commands::signOut:
        {
            if (status.isUnlocked())
            {
                auto* props = settings.getUserSettings();
                props->removeValue("L");
                props->save();
                props->reload();
                status.load();
            }
        } break;
        
        case Commands::checkNewerVersion:
            CurrentVersion::checkAfterDelay (20, true);
            break;
        
        default: res = false; break;
    }
    return res;
}

}
