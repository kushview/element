
#include "controllers/AppController.h"
#include "controllers/EngineController.h"
#include "controllers/GuiController.h"
#include "engine/GraphProcessor.h"
#include "gui/GuiApp.h"
#include "gui/UnlockForm.h"
#include "session/UnlockStatus.h"
#include "Globals.h"
#include "Messages.h"
#include "Settings.h"

namespace Element {

AppController::AppController (Globals& g)
    : world (g)
{
    gui = GuiApp::create (g, *this);
    addChild (new GuiController());
    addChild (new EngineController());
    g.getCommandManager().registerAllCommandsForTarget (this);
    g.getCommandManager().setFirstCommandTarget (this);
}

AppController::~AppController()
{
    gui->closeAllWindows();
    gui = nullptr;
}

void AppController::deactivate()
{
    gui->closeAllWindows();
    Controller::deactivate();
}
    
void AppController::run()
{
    activate();
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
    return gui.get();
}

void AppController::getAllCommands (Array<CommandID>& cids)
{
    cids.addArray ({
        Commands::mediaNew,  Commands::mediaOpen,
        Commands::mediaSave, Commands::mediaSaveAs,
        Commands::signIn,    Commands::signOut
    });
}

void AppController::getCommandInfo (CommandID command, ApplicationCommandInfo& result)
{
    if (gui)
        gui->getCommandInfo (command, result);
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
                GraphProcessor& graph (world.getAudioEngine()->graph());
                
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
                GraphProcessor& graph (world.getAudioEngine()->graph());
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
