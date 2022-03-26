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

#include "controllers/AppController.h"
#include "controllers/GuiController.h"
#include "gui/ContentComponentPro.h"
#include "gui/workspace/VirtualKeyboardPanel.h"
#include "gui/workspace/WorkspacePanel.h"
#include "gui/Workspace.h"
#include "Globals.h"
#include "BinaryData.h"

namespace Element {

class ContentComponentPro::Impl
{
public:
    Impl (ContentComponentPro& o, AppController& a)
        : app (a), owner (o), workspace (a.getGlobals(), a, *a.findChild<GuiController>())
    {
        owner.addAndMakeVisible (workspace);
    }

    ~Impl()
    {
    }

    Dock& getDock() { return workspace.getDock(); }

    void resized (const Rectangle<int>& area)
    {
        workspace.setBounds (area);
    }

    void stabilizePanels()
    {
        for (int i = 0; i < workspace.getDock().getNumPanels(); ++i)
        {
            auto* const panel = getDock().getPanel (i);
            if (auto* const workspacePanel = dynamic_cast<WorkspacePanel*> (panel))
            {
                workspacePanel->stabilizeContent();
            }
        }
    }

    template <class PanelType>
    PanelType* findPanel() const noexcept
    {
        auto* me = const_cast<Impl*> (this);
        auto& dock = me->getDock();
        for (int i = 0; i < dock.getNumPanels(); ++i)
            if (auto* const panel = dynamic_cast<PanelType*> (dock.getPanel (i)))
                return panel;
        return nullptr;
    }

    AppController& app;
    ContentComponentPro& owner;
    Workspace workspace;
};

ContentComponentPro::ContentComponentPro (AppController& controller)
    : ContentComponent (controller)
{
    impl.reset (new Impl (*this, controller));
}

ContentComponentPro::~ContentComponentPro() noexcept
{
    impl.reset (nullptr);
}

void ContentComponentPro::resizeContent (const Rectangle<int>& area)
{
    impl->resized (area);
}

void ContentComponentPro::stabilize (const bool refreshDataPathTrees)
{
    impl->stabilizePanels();
}

void ContentComponentPro::stabilizeViews()
{
    impl->stabilizePanels();
}

String ContentComponentPro::getWorkspaceName() const
{
    return impl->workspace.getName();
}

WorkspaceState ContentComponentPro::getWorkspaceState()
{
    WorkspaceState state (impl->workspace);
    return state;
}

void ContentComponentPro::applyWorkspaceState (const WorkspaceState& state)
{
    auto& workspace = impl->workspace;
    workspace.applyState (state);
}

void ContentComponentPro::addWorkspaceItemsToMenu (PopupMenu& menu)
{
    auto& dock = impl->getDock();
    const int offset = 100000;
    int index = 0;
    for (const auto* const desc : dock.getPanelDescriptions())
    {
        if (! desc->showInMenu)
        {
            ++index; // need to do this so result still matches the description
            continue;
        }
        menu.addItem (offset + index++, desc->name);
    }
}

void ContentComponentPro::handleWorkspaceMenuResult (int result)
{
    auto& dock = impl->getDock();
    const int index = result - 100000;
    const auto& descs = dock.getPanelDescriptions();

    if (isPositiveAndBelow (index, descs.size()))
    {
        const auto panelId = descs[index]->identifier;

        if (auto* const selected = dock.getSelectedItem())
        {
            if (auto* const item = dock.createItem (panelId))
                item->dockTo (selected, DockPlacement::Center);
        }
        else
        {
            dock.createItem (panelId, DockPlacement::Top);
        }
    }
}

void ContentComponentPro::saveState (PropertiesFile*)
{
}

void ContentComponentPro::restoreState (PropertiesFile*)
{
}

void ContentComponentPro::getSessionState (String&)
{
}

void ContentComponentPro::applySessionState (const String&)
{
}

//==============================================================================
bool ContentComponentPro::isVirtualKeyboardVisible() const
{
    if (auto* panel = impl->findPanel<VirtualKeyboardPanel>())
        return panel->isVisible();
    return false;
}

void ContentComponentPro::setVirtualKeyboardVisible (const bool isVisible)
{
    ContentComponent::setVirtualKeyboardVisible (isVisible);
}

void ContentComponentPro::toggleVirtualKeyboard()
{
    ContentComponent::toggleVirtualKeyboard();
}

VirtualKeyboardView* ContentComponentPro::getVirtualKeyboardView() const
{
    if (auto* panel = impl->findPanel<VirtualKeyboardPanel>())
        return dynamic_cast<VirtualKeyboardView*> (&panel->getView());
    return nullptr;
}

} // namespace Element
