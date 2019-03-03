
#include "controllers/AppController.h"
#include "controllers/EngineController.h"
#include "controllers/DevicesController.h"
#include "controllers/MappingController.h"
#include "controllers/GuiController.h"
#include "controllers/PresetsController.h"
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
    document->setLastDocumentOpened (DataPath::defaultSessionDir().getChildFile ("Untitled.els"));
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
    refreshOtherControllers();
    findSibling<GuiController>()->stabilizeContent();
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
            model.forEach ([](const ValueTree& tree)
            {
                if (! tree.hasType (Tags::node))
                    return;
                auto nodeRef = tree;
                nodeRef.setProperty (Tags::uuid, Uuid().toString(), nullptr);
            });
            if (auto* ec = findSibling<EngineController>())
                ec->addGraph (model);
        }
    }
    else if (file.hasFileExtension ("els"))
    {
        document->saveIfNeededAndUserAgrees();
        Session::ScopedFrozenLock freeze (*currentSession);
        Result result = document->loadFrom (file, true);
        
        if (result.wasOk())
        {
            if (auto* gc = findSibling<GuiController>())
                gc->closeAllPluginWindows();
            refreshOtherControllers();
            findSibling<GuiController>()->stabilizeContent();
            resetChanges();
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

void SessionController::resetChanges (const bool resetDocumentFile)
{
    jassert (document);
    if (resetDocumentFile)
        document->setFile ({});
    document->setChangedFlag (false);
    jassert (! document->hasChangedSinceSaved());
}

void SessionController::saveSession (const bool saveAs)
{
    jassert (document && currentSession);
    auto result = FileBasedDocument::userCancelledSave;

    if (saveAs) {
        result = document->saveAs (File(), true, true, true);
    } else {
        result = document->save (true, true);
    }

    if (result == FileBasedDocument::userCancelledSave)
        return;
    
    if (result == FileBasedDocument::savedOk)
    {
        // ensure change messages are flushed so the changed flag doesn't reset
        currentSession->dispatchPendingMessages();
        document->setChangedFlag (false);
        jassert (! hasSessionChanged());
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
        findSibling<GuiController>()->closeAllPluginWindows();
        loadNewSessionData();
        refreshOtherControllers();
        findSibling<GuiController>()->stabilizeContent();
        resetChanges (true);
    }
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

void SessionController::refreshOtherControllers()
{
    findSibling<EngineController>()->sessionReloaded();
    findSibling<DevicesController>()->refresh();
    findSibling<MappingController>()->learn (false);
    findSibling<PresetsController>()->refresh();
    sessionLoaded();
}

}
