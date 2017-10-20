
#include "controllers/AppController.h"
#include "controllers/EngineController.h"
#include "controllers/GuiController.h"
#include "controllers/SessionController.h"
#include "session/Node.h"
#include "Globals.h"
#include "Settings.h"

namespace Element {

void SessionController::activate()
{
    auto* app = dynamic_cast<AppController*> (getRoot());
    currentSession = app->getWorld().getSession();
    document = new SessionDocument (currentSession);
}

void SessionController::deactivate()
{
    auto& world = getWorld();
    auto& settings (world.getSettings());
    auto* props = settings.getUserSettings();
    
    if (document)
    {
        if (document->getFile().existsAsFile())
            props->setValue ("lastSession", document->getFile().getFullPathName());
        document = nullptr;
    }
    
    currentSession->clear();
    currentSession = nullptr;
}

void SessionController::openFile (const File& file)
{
    bool didSomething = true;
    
    if (file.hasFileExtension ("elg"))
    {
        const ValueTree node (Node::parse (file));
        if (Node::isProbablyGraphNode (node))
        {
            const Node model (node, true);
            DBG("[EL] add graph to session: " << model.getName());
            currentSession->addGraph (node, true);
        }
    }
    else if (file.hasFileExtension ("els"))
    {
        DBG("[El] opening session " << file.getFileName());
        Result result = document->loadFrom (file, true);
        if (result.wasOk())
        {
            currentSession->freezeChangeNotification = true;
            if (auto* ec = findSibling<EngineController>())
                ec->setRootNode (currentSession->getCurrentGraph());
            currentSession->freezeChangeNotification = false;
        }
    }
    else
    {
        didSomething = false;
    }
    
    if (didSomething)
        if (auto* gc = findSibling<GuiController>())
            gc->stabilizeContent();
}

void SessionController::exportGraph (const Node& node, const File& targetFile)
{
    if (! node.hasNodeType (Tags::graph)) {
        jassertfalse;
        return;
    }
    
    TemporaryFile tempFile (targetFile);
    if (node.writeToFile (tempFile.getFile()))
        tempFile.overwriteTargetFileWithTemporary();
}

void SessionController::importGraph (const File& file)
{
    openFile (file);
}

void SessionController::closeSession()
{
    DBG("[SC] close session");
}

void SessionController::saveSession (const bool saveAs)
{
    jassert(document && currentSession);
    
    if (saveAs) {
        document->saveAs (File::nonexistent, true, true, true);
    } else {
        document->save (true, true);
    }
}

void SessionController::newSession()
{
    jassert (document && currentSession);
    // - 0 if the third button was pressed ('cancel')
    // - 1 if the first button was pressed ('yes')
    // - 2 if the middle button was pressed ('no')
    int res = 2;
    if (document->hasChangedSinceSaved())
        res = AlertWindow::showYesNoCancelBox (AlertWindow::InfoIcon, "Save Session?",
                                               "The current session has changes. Would you like to save it?",
                                               "Save Session", "Don't Save", "Cancel");
    if (res == 1)
        document->save (true, true);
    
    if (res == 1 || res == 2)
    {
        currentSession->freezeChangeNotification = true;
        currentSession->clear();
        if (auto* ec = findSibling<EngineController>())
            if (currentSession->getNumGraphs() > 0)
                ec->setRootNode (currentSession->getCurrentGraph());
            else
                ec->clear();
        if (auto* gc = findSibling<GuiController>())
            gc->stabilizeContent();
        document = new SessionDocument (currentSession);
        currentSession->freezeChangeNotification = false;
    }
}

}
