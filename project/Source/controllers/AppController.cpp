
#include "controllers/AppController.h"
#include "controllers/EngineController.h"
#include "controllers/GuiController.h"
#include "engine/GraphProcessor.h"
#include "gui/UnlockForm.h"
#include "session/UnlockStatus.h"
#include "Commands.h"
#include "Globals.h"
#include "Messages.h"
#include "Settings.h"

namespace Element {

AppController::AppController (Globals& g)
    : world (g)
{
    addChild (new GuiController (g, *this));
    addChild (new EngineController ());
    g.getCommandManager().registerAllCommandsForTarget (this);
    g.getCommandManager().setFirstCommandTarget (this);
}

AppController::~AppController()
{

}

void AppController::deactivate()
{
    Controller::deactivate();
}

void AppController::run()
{
    activate();
    if (auto* gui = findChild<GuiController>())
        gui->run();
}

void AppController::handleMessage (const Message& msg)
{
	auto* ec = findChild<EngineController>();
	jassert(ec);

    bool handled = true;

    if (const auto* lpm = dynamic_cast<const LoadPluginMessage*> (&msg))
    {
        ec->addPlugin (lpm->description);
    }
    else if (const auto* rnm = dynamic_cast<const RemoveNodeMessage*> (&msg))
    {
        ec->removeNode (rnm->nodeId);
    }
    else if (const auto* acm = dynamic_cast<const AddConnectionMessage*> (&msg))
    {
        if (acm->useChannels())
            ec->connectChannels (acm->sourceNode, acm->sourceChannel, acm->destNode, acm->destChannel);
        else
            ec->addConnection (acm->sourceNode, acm->sourcePort, acm->destNode, acm->destPort);
    }
    else if (const auto* rcm = dynamic_cast<const RemoveConnectionMessage*> (&msg))
    {
        if (rcm->useChannels())
		{
            jassertfalse; // channels not yet supported
        }
        else
        {
            ec->removeConnection (rcm->sourceNode, rcm->sourcePort, rcm->destNode, rcm->destPort);
        }
    }
    else
    {
        handled = false;
    }
    
    if (! handled)
    {
        DBG("[EL] AppController: unhandled Message received");
    }
}


ApplicationCommandTarget* AppController::getNextCommandTarget()
{
    return findChild<GuiController>();
}

void AppController::getAllCommands (Array<CommandID>& cids)
{
    cids.addArray ({
        Commands::mediaNew,  Commands::mediaOpen,
        Commands::mediaSave, Commands::mediaSaveAs,
        Commands::signIn,    Commands::signOut
    });
}

void AppController::getCommandInfo (CommandID commandID, ApplicationCommandInfo& result)
{
    if (Commands::devicePadPress <= commandID && (Commands::devicePadPress + 13) > commandID)
    {
        result.setInfo (("Pad Press"), "Triggers sounds.", "Beat Thang Hardware", ApplicationCommandInfo::dontTriggerVisualFeedback);
        result.addDefaultKeypress ('A', ModifierKeys::noModifiers);
    }
    else if (Commands::devicePadRelease <= commandID && (Commands::devicePadRelease + 13) > commandID)
        result.setInfo (("Pad Release"), "Ends playing sounds.", "Beat Thang Hardware", 0);
    
    if (result.description.isNotEmpty())
        return;
    
    if (Commands::getDeviceTrackInfo (commandID, result))
        return;
    
    switch (commandID)
    {
        case Commands::exportAudio:
            result.setInfo ("Export Audio", "Export to an audio file", "Exporting", 0);
            break;
        case Commands::exportMidi:
            result.setInfo ("Exort MIDI", "Export to a MIDI file", "Exporting", 0);
            break;
            
        case Commands::sessionClose:
            //            result.addDefaultKeypress ('w', ModifierKeys::commandModifier);
            result.setInfo ("Close Session", "Close the current session", "Session", 0);
            break;
        case Commands::sessionNew:
            //            result.addDefaultKeypress ('n', ModifierKeys::commandModifier);
            result.setInfo ("New Session", "Create a new session", "Session", 0);
            break;
        case Commands::sessionOpen:
            //            result.addDefaultKeypress ('o', ModifierKeys::commandModifier);
            result.setInfo ("Open Session", "Open an existing session", "Session", 0);
            break;
        case Commands::sessionSave:
            //            result.addDefaultKeypress ('s', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setInfo ("Save Session", "Save the current session", "Session", 0);
            break;
        case Commands::sessionSaveAs:
            //            result.addDefaultKeypress ('s', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setInfo ("Save Session As", "Save the current session with a new name", "Session", 0);
            break;
            
        case Commands::showPreferences:
            result.setInfo ("Show Preferences", "Element Preferences", "Application", 0);
            result.addDefaultKeypress (',', ModifierKeys::commandModifier);
            break;
            
        case Commands::showAbout:
            result.setInfo ("Show About", "About this program", "Application", 0);
            break;
        case Commands::showLegacyView:
            result.setInfo ("Legacy View", "Shows the legacy Beat Thang Virtual GUI", "Interfaces", 0);
            break;
        case Commands::showPluginManager:
            result.setInfo ("Plugin Manager", "Element Plugin Management", "Application", 0);
            break;
        case Commands::checkNewerVersion:
            result.setInfo ("Check For Updates", "Check newer version", "Application", 0);
            break;
            
        case Commands::mediaNew:
            result.addDefaultKeypress ('n', ModifierKeys::commandModifier);
            result.setInfo ("New Media", "Close the current media", "Application", 0);
            break;
        case Commands::mediaClose:
            result.setInfo ("Close Media", "Close the current media", "Application", 0);
            break;
        case Commands::mediaOpen:
            result.addDefaultKeypress ('o', ModifierKeys::commandModifier);
            result.setInfo ("Open Media", "Opens a type of supported media", "Session Media", 0);
            break;
        case Commands::mediaSave:
            result.addDefaultKeypress ('s', ModifierKeys::commandModifier);
            result.setInfo ("Close Media", "Saves the currently viewed object", "Session Media", 0);
            break;
        case Commands::mediaSaveAs:
            result.addDefaultKeypress ('s', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setInfo ("Close Media", "Saves the current object with another name", "Session Media", 0);
            break;
            
        case Commands::signIn:
            result.setInfo ("Sign In", "Saves the current object with another name", "Application", 0);
            break;
        case Commands::signOut:
            result.setInfo ("Sign Out", "Saves the current object with another name", "Application",   0);
            break;
            
        case Commands::quit:
            result.setActive (false);
            result.setInfo("Quit", "Quit the app", "Application", 0);
            result.addDefaultKeypress ('q', ModifierKeys::commandModifier);
            break;
        case Commands::undo:
            result.setInfo ("Undo", "Undo the last operation", "Application", 0);
            break;
        case Commands::redo:
            result.setInfo ("Redo", "Redo the last operation", "Application", 0);
            break;
        case Commands::cut:
            result.setInfo ("Cut", "Cut", "Application", 0);
            break;
        case Commands::copy:
            result.setInfo ("Copy", "Copy", "Application", 0);
            break;
        case Commands::paste:
            result.setInfo ("Paste", "Paste", "Application", 0);
            break;
        case Commands::selectAll:
            result.setInfo ("Select All", "Select all", "Application", 0);
            break;
            
        case Commands::transportRewind:
            result.setInfo ("Rewind", "Transport Rewind", "Playback", 0);
            result.addDefaultKeypress ('j', 0);
            break;
        case Commands::transportForward:
            result.setInfo ("Forward", "Transport Fast Forward", "Playback", 0);
            result.addDefaultKeypress ('l', 0);
            break;
        case Commands::transportPlay:
            result.setInfo ("Play", "Transport Play", "Playback", 0);
            result.addDefaultKeypress (KeyPress::spaceKey, 0);
            break;
        case Commands::transportRecord:
            result.setInfo ("Record", "Transport Record", "Playback", 0);
            break;
        case Commands::transportSeekZero:
            result.setInfo ("Seek Start", "Seek to Beginning", "Playback", 0);
            break;
        case Commands::transportStop:
            result.setInfo ("Stop", "Transport Stop", "Playback", 0);
            break;
    }
}

bool AppController::perform (const InvocationInfo& info)
{
    EngineController* ec = findChild<EngineController>();
    jassert (ec);
    
    auto& status (world.getUnlockStatus());
	auto& settings(getGlobals().getSettings());
    bool res = true;
    switch (info.commandID)
    {
        case Commands::mediaNew:
		{
            if (AlertWindow::showOkCancelBox (AlertWindow::InfoIcon, "New Graph", 
											  "This will clear the current graph, are you sure?"))
            {
                lastSavedFile = File();
                ec->clear();
            }
        } break;

        case Commands::mediaOpen:
        {
            FileChooser chooser ("Open a graph", File(), "*.elgraph;*.elg");
            if (chooser.browseForFileToOpen())
            {
                lastSavedFile = chooser.getResult();
                FileInputStream stream (lastSavedFile);
                const ValueTree g (ValueTree::readFromStream (stream));
                if (g.isValid())
                {
                    const Node node (g, false);
                    ec->setRootNode (node);
                }
            }
        } break;
            
        case Commands::mediaSave:
        {
            if (! status.isUnlocked())
            {
                AlertWindow::showMessageBox (AlertWindow::InfoIcon,
                    "Unauthorized", "Saving is only available with a paid version of this software. Visit https://kushview.net/products/element to purchase a copy");
            }
            else
            {
                auto& graph (world.getAudioEngine()->getRootGraph());
                
                if (lastSavedFile.existsAsFile() && lastSavedFile.hasFileExtension ("elgraph"))
                {
                    const ValueTree model (graph.getGraphState());
                    FileOutputStream stream (lastSavedFile);
                    model.writeToStream (stream);
                }
                
                else
                {
                    FileChooser chooser ("Save current graph", File(), "*.elgraph");
                    if (chooser.browseForFileToSave (true))
                    {
                        lastSavedFile = chooser.getResult();
                        const ValueTree model (graph.getGraphState());
                        FileOutputStream stream (lastSavedFile);
                        model.writeToStream (stream);
                    }
                }
            }
        } break;
        
        case Commands::mediaSaveAs: {
            if (! status.isUnlocked())
            {
                AlertWindow::showMessageBox (AlertWindow::InfoIcon,
                    "Unauthorized", "Saving is only available with a paid version of this software. Visit https://kushview.net/products/element to purchase a copy");
            }
            else
            {
                GraphProcessor& graph (world.getAudioEngine()->getRootGraph());
                FileChooser chooser ("Save current graph", File(), "*.elgraph");
                if (chooser.browseForFileToSave (true))
                {
                    lastSavedFile = chooser.getResult();
                    const ValueTree model (graph.getGraphState());
                    FileOutputStream stream (lastSavedFile);
                    model.writeToStream (stream);
                }
            }
        } break;
        
        case Commands::signIn:
        {
            {
                auto* form = new UnlockForm (status, "Enter your license key.",
                                             false, false, true, true);
                DialogWindow::LaunchOptions opts;
                opts.content.setOwned (form);
                opts.resizable = false;
                opts.dialogTitle = "Authorization";
                opts.runModal();
            }
        } break;
        
        case Commands::signOut:
        {
           
            if (status.isUnlocked())
            {
                auto* props = settings.getUserSettings();
                props->removeValue("L");
                props->save();
                status.load();
            }
        } break;
        
        default: res = false; break;
    }
    return res;
}

}
