
#include "controllers/DevicesController.h"
#include "controllers/EngineController.h"
#include "controllers/GraphController.h"
#include "controllers/GuiController.h"
#include "controllers/MappingController.h"
#include "controllers/PresetsController.h"
#include "gui/SessionImportWizard.h"
#include "session/Session.h"
#include "DataPath.h"
#include "Globals.h"
#include "Settings.h"

namespace Element {

void GraphController::activate()
{
    document.setLastDocumentOpened (
        DataPath::defaultGraphDir().getChildFile ("Untitled.elg"));
}

void GraphController::deactivate()
{
    wizard.reset();
    if (auto* const props = getWorld().getSettings().getUserSettings())
        if (document.getFile().existsAsFile())
            props->setValue (Settings::lastGraphKey, document.getFile().getFullPathName());
}

void GraphController::openDefaultGraph()
{
    GraphDocument::ScopedChangeStopper freeze (document, false);
    if (auto* gc = findSibling<GuiController>())
        gc->closeAllPluginWindows();
    
    getWorld().getSession()->clear();
    auto newGraph = Node::createDefaultGraph();
    document.setGraph (newGraph);
    getWorld().getSession()->addGraph (document.getGraph(), true);
    graphChanged();

    refreshOtherControllers();
    findSibling<GuiController>()->stabilizeContent();
}

void GraphController::openGraph (const File& file)
{
    if (file.hasFileExtension ("els"))
    {
        if (wizard != nullptr)
            wizard.reset();
        auto* const dialog = new SessionImportWizardDialog (wizard, file);
        dialog->onGraphChosen = std::bind (&GraphController::loadGraph, this, std::placeholders::_1);
        return;
    }

    auto result = document.loadFrom (file, true);
    
    if (result.wasOk())
    {
        GraphDocument::ScopedChangeStopper freeze (document, false);
        findSibling<GuiController>()->closeAllPluginWindows();

        getWorld().getSession()->clear();
        getWorld().getSession()->addGraph (document.getGraph(), true);
        graphChanged();

        refreshOtherControllers();
        findSibling<GuiController>()->stabilizeContent();
    }
}

void GraphController::loadGraph (const Node& graph)
{
    document.saveIfNeededAndUserAgrees();
    document.setGraph (graph);
    document.setFile (File());

    GraphDocument::ScopedChangeStopper freeze (document, false);
    findSibling<GuiController>()->closeAllPluginWindows();
    getWorld().getSession()->clear();
    getWorld().getSession()->addGraph (document.getGraph(), true);
    
    graphChanged();

    refreshOtherControllers();
    findSibling<GuiController>()->stabilizeContent();
}

void GraphController::newGraph()
{
    // - 0 if the third button was pressed ('cancel')
    // - 1 if the first button was pressed ('yes')
    // - 2 if the middle button was pressed ('no')
    int res = 2;
    if (document.hasChangedSinceSaved())
        res = AlertWindow::showYesNoCancelBox (AlertWindow::InfoIcon, "Save Graph?",
                                               "The current graph has changes. Would you like to save it?",
                                               "Save Graph", "Don't Save", "Cancel");
    if (res == 1)
        document.save (true, true);
    
    if (res == 1 || res == 2)
    {
        GraphDocument::ScopedChangeStopper freeze (document, false);
        findSibling<GuiController>()->closeAllPluginWindows();

        getWorld().getSession()->clear();
        auto newGraph = Node::createDefaultGraph();
        document.setGraph (newGraph);
        document.setFile (File());
        getWorld().getSession()->addGraph (document.getGraph(), true);
        graphChanged();

        refreshOtherControllers();
        findSibling<GuiController>()->stabilizeContent();
    }
}

void GraphController::saveGraph (const bool saveAs)
{
    auto result = FileBasedDocument::userCancelledSave;

    if (saveAs) {
        result = document.saveAs (File(), true, true, true);
    } else {
        result = document.save (true, true);
    }

    if (result == FileBasedDocument::userCancelledSave)
        return;
    
    if (result == FileBasedDocument::savedOk)
    {
        // ensure change messages are flushed so the changed flag doesn't reset
        document.setChangedFlag (false);
        jassert (! hasGraphChanged());
    }
}

void GraphController::refreshOtherControllers()
{
    findSibling<EngineController>()->sessionReloaded();
    findSibling<DevicesController>()->refresh();
    findSibling<MappingController>()->learn (false);
    findSibling<PresetsController>()->refresh();
}

}
