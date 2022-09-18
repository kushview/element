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

#include "services/deviceservice.hpp"
#include "services/engineservice.hpp"
#include "services/graphservice.hpp"
#include "services/guiservice.hpp"
#include "services/mappingservice.hpp"
#include "services/presetservice.hpp"
#include "gui/ContentComponent.h"
#include "gui/SessionImportWizard.h"
#include "session/session.hpp"
#include "datapath.hpp"
#include "context.hpp"
#include "settings.hpp"

namespace element {

void GraphService::activate()
{
    document.setSession (getWorld().getSession());
    document.setLastDocumentOpened (
        DataPath::defaultGraphDir().getChildFile ("Untitled.elg"));
}

void GraphService::deactivate()
{
    wizard.reset();
    if (auto* const props = getWorld().getSettings().getUserSettings())
        if (document.getFile().existsAsFile())
            props->setValue (Settings::lastGraphKey, document.getFile().getFullPathName());
}

void GraphService::openDefaultGraph()
{
    GraphDocument::ScopedChangeStopper freeze (document, false);
    if (auto* gc = findSibling<GuiService>())
        gc->closeAllPluginWindows();

    auto newGraph = Node::createDefaultGraph();
    document.setGraph (newGraph);
    graphChanged();

    refreshOtherControllers();
    findSibling<GuiService>()->stabilizeContent();
}

void GraphService::openGraph (const File& file)
{
    if (file.hasFileExtension ("els"))
    {
        if (wizard != nullptr)
            wizard.reset();
        auto* const dialog = new SessionImportWizardDialog (wizard, file);
        dialog->onGraphChosen = std::bind (&GraphService::loadGraph, this, std::placeholders::_1);
        return;
    }

    auto result = document.loadFrom (file, true);

    if (result.wasOk())
    {
        auto& gui = *findSibling<GuiService>();
        GraphDocument::ScopedChangeStopper freeze (document, false);
        findSibling<GuiService>()->closeAllPluginWindows();
        graphChanged();
        refreshOtherControllers();

        auto session = getWorld().getSession();

        if (auto* const cc = gui.getContentComponent())
        {
            auto ui = session->getValueTree().getOrCreateChildWithName (Tags::ui, nullptr);
            cc->applySessionState (ui.getProperty ("content").toString());
        }

        findSibling<GuiService>()->stabilizeContent();
        getServices().addRecentFile (file);
    }
}

void GraphService::loadGraph (const Node& graph)
{
    document.saveIfNeededAndUserAgrees();
    document.setGraph (graph);
    document.setFile (File());

    GraphDocument::ScopedChangeStopper freeze (document, false);
    findSibling<GuiService>()->closeAllPluginWindows();
    graphChanged();
    refreshOtherControllers();
    findSibling<GuiService>()->stabilizeContent();
}

void GraphService::newGraph()
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
        findSibling<GuiService>()->closeAllPluginWindows();

        auto newGraph = Node::createDefaultGraph();
        document.setGraph (newGraph);
        document.setFile (File());
        graphChanged();

        refreshOtherControllers();
        findSibling<GuiService>()->stabilizeContent();
    }
}

void GraphService::saveGraph (const bool saveAs)
{
    auto result = FileBasedDocument::userCancelledSave;
    auto session = getWorld().getSession();
    auto& gui = *findSibling<GuiService>();

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
        if (saveAs)
        {
            getServices().addRecentFile (document.getFile());
            document.getGraph().setProperty (Tags::name,
                                             document.getFile().getFileNameWithoutExtension());
        }
    }
}

void GraphService::refreshOtherControllers()
{
    findSibling<EngineService>()->sessionReloaded();
    findSibling<DeviceService>()->refresh();
    findSibling<MappingService>()->learn (false);
    findSibling<PresetService>()->refresh();
}

} // namespace element
