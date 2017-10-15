
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
    auto& world = (dynamic_cast<AppController*>(getRoot()))->getWorld();
    auto& settings (world.getSettings());
    auto* props = settings.getUserSettings();
    
    if (document)
    {
        if (document->getFile().existsAsFile())
            props->setValue ("lastSession", document->getFile().getFullPathName());
        document = nullptr;
    }
    
    currentSession = nullptr;
}

void SessionController::openFile (const File& file)
{
    if (file.hasFileExtension ("elg"))
    {
        if (ScopedPointer<XmlElement> e = XmlDocument::parse (file))
        {
            const ValueTree node (ValueTree::fromXml (*e));
            if (node.hasType (Tags::graph))
            {
                DBG("[EL] add graph to session");
                const Node model (node, true);
            }
        }
    }
    else if (file.hasFileExtension ("els"))
    {
        DBG("[El] opening session " << file.getFileName());
        Result result = document->loadFrom (file, true);
        if (result.wasOk())
        {
            if (auto* ec = findSibling<EngineController>())
                ec->setRootNode (Node (currentSession->getGraphValueTree(0), false));
            if (auto* gc = findSibling<GuiController>())
                gc->stabilizeContent();
        }
    }
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
        currentSession->clear();
        if (auto* ec = findSibling<EngineController>())
            ec->setRootNode (Node (currentSession->getGraphValueTree(0), false));
        if (auto* gc = findSibling<GuiController>())
            gc->stabilizeContent();
        document->setFile (File::nonexistent);
        document->setChangedFlag (false);
    }
}

}
