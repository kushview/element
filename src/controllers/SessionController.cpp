
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

void SessionController::openDefaultSession()
{
    if (auto* gc = findSibling<GuiController>())
        gc->closeAllPluginWindows();
        
    loadNewSessionData();
        
    if (auto* ec = findSibling<EngineController>())
        ec->sessionReloaded();
    if (auto* gc = findSibling<GuiController>())
        gc->stabilizeContent();

    resetChanges();
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
            if (auto* ec = findSibling<EngineController>())
                ec->addGraph (model);
        }
    }
    else if (file.hasFileExtension ("els"))
    {
        DBG("[El] opening session " << file.getFullPathName());
        document->saveIfNeededAndUserAgrees();
        Session::ScopedFrozenLock freeze (*currentSession);
        Result result = document->loadFrom (file, true);
        
        if (result.wasOk())
        {
            if (auto* gc = findSibling<GuiController>())
                gc->closeAllPluginWindows();
            if (auto* ec = findSibling<EngineController>())
                ec->sessionReloaded();
            
            // controllers handling session reload might change the model
            // this needs to happen after other controllers are finished
            document->setChangedFlag (false);
            DBG("[EL] opened: " << document->getFile().getFullPathName());
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
    jassert (document && currentSession);
    DBG("getFile(): " << document->getFile().getFullPathName());
    DBG("getLastDocumentOpened(): " << document->getLastDocumentOpened().getFullPathName());
    DBG("hasChangedSinceSaved(): " << (int) document->hasChangedSinceSaved());
    
    if (saveAs) {
        document->saveAs (File(), true, true, true);
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
        if (auto* gc = findSibling<GuiController>())
            gc->closeAllPluginWindows();
        
        loadNewSessionData();
        
        if (auto* ec = findSibling<EngineController>())
            ec->sessionReloaded();
        if (auto* gc = findSibling<GuiController>())
            gc->stabilizeContent();
    }

    resetChanges();
}

void SessionController::loadNewSessionData()
{
    currentSession->clear();
    const auto file = getWorld().getSettings().getDefaultNewSessionFile();
    bool wasLoaded = false;

    if (file.existsAsFile())
    {
        ValueTree data;
        if (ScopedPointer<XmlElement> xml = XmlDocument::parse (file))
            data = ValueTree::fromXml (*xml);
        if (data.isValid() && data.hasType (Tags::session))
            wasLoaded = currentSession->loadData (data);
    }

    if (! wasLoaded)
    {   
        currentSession->clear();
        currentSession->addGraph (Node::createDefaultGraph(), true);
    }
}

}
