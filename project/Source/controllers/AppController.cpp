
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
        Commands::signIn,    Commands::signOut,
        Commands::sessionNew
    });
}

void AppController::getCommandInfo (CommandID commandID, ApplicationCommandInfo& result)
{
    Commands::getCommandInfo (commandID, result);
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
        case Commands::sessionNew:
            findChild<GuiController>()->newSession();
            break;
        
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
            if (! status.isFullVersion())
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
            if (! status.isFullVersion())
            {
                String msg = "Saving is available in Element Pro.\n";
                msg << "Visit https://kushview.net/products/element to purchase a copy";
                AlertWindow::showMessageBox (AlertWindow::InfoIcon, "Unauthorized", msg);
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
                opts.dialogTitle = "License Manager";
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
