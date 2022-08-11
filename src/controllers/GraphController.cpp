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

#include "controllers/DevicesController.h"
#include "controllers/EngineController.h"
#include "controllers/GraphController.h"
#include "controllers/GuiController.h"
#include "controllers/MappingController.h"
#include "controllers/PresetsController.h"
#include "gui/ContentComponent.h"
#include "gui/SessionImportWizard.h"
#include "session/Session.h"
#include "datapath.hpp"
#include "Globals.h"
#include "Settings.h"

namespace Element {

void GraphController::activate()
{
    document.setSession (getWorld().getSession());
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

    auto newGraph = Node::createDefaultGraph();
    document.setGraph (newGraph);
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
        auto& gui = *findSibling<GuiController>();
        GraphDocument::ScopedChangeStopper freeze (document, false);
        findSibling<GuiController>()->closeAllPluginWindows();
        graphChanged();
        refreshOtherControllers();

        auto session = getWorld().getSession();

        if (auto* const cc = gui.getContentComponent())
        {
            auto ui = session->getValueTree().getOrCreateChildWithName (Tags::ui, nullptr);
            cc->applySessionState (ui.getProperty ("content").toString());
        }

        findSibling<GuiController>()->stabilizeContent();
        getAppController().addRecentFile (file);
    }
}

void GraphController::loadGraph (const Node& graph)
{
    document.saveIfNeededAndUserAgrees();
    document.setGraph (graph);
    document.setFile (File());

    GraphDocument::ScopedChangeStopper freeze (document, false);
    findSibling<GuiController>()->closeAllPluginWindows();
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
        res = AlertWindow::showYesNoCancelBox (AlertWindow::InfoIcon, "Save Graph?", "The current graph has changes. Would you like to save it?", "Save Graph", "Don't Save", "Cancel");
    if (res == 1)
        document.save (true, true);

    if (res == 1 || res == 2)
    {
        GraphDocument::ScopedChangeStopper freeze (document, false);
        findSibling<GuiController>()->closeAllPluginWindows();

        auto newGraph = Node::createDefaultGraph();
        document.setGraph (newGraph);
        document.setFile (File());
        graphChanged();

        refreshOtherControllers();
        findSibling<GuiController>()->stabilizeContent();
    }
}

void GraphController::saveGraph (const bool saveAs)
{
    auto result = FileBasedDocument::userCancelledSave;
    auto session = getWorld().getSession();
    auto& gui = *findSibling<GuiController>();

    if (auto* const cc = gui.getContentComponent())
    {
        String state;
        cc->getSessionState (state);
        auto ui = session->getValueTree().getOrCreateChildWithName (Tags::ui, nullptr);
        ui.setProperty ("content", state, nullptr);
    }

    if (saveAs)
    {
        result = document.saveAsInteractive (true);
        if (result == FileBasedDocument::savedOk)
            document.getGraph().setProperty (Tags::name, 
                document.getFile().getFileNameWithoutExtension());
    }
    else
    {
        result = document.save (true, true);
    }

    if (result == FileBasedDocument::userCancelledSave)
        return;

    if (result == FileBasedDocument::savedOk)
    {
        // ensure change messages are flushed so the changed flag doesn't reset
        document.setChangedFlag (false);
        jassert (! hasGraphChanged());
        if (auto* us = getWorld().getSettings().getUserSettings())
            us->setValue (Settings::lastGraphKey, document.getFile().getFullPathName());
    }
}

void GraphController::refreshOtherControllers()
{
    findSibling<EngineController>()->sessionReloaded();
    findSibling<DevicesController>()->refresh();
    findSibling<MappingController>()->learn (false);
    findSibling<PresetsController>()->refresh();
}

} // namespace Element
