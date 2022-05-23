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
#include "gui/WorkspacesContentComponent.h"
#include "gui/workspace/PanelTypes.h"
#include "gui/workspace/VirtualKeyboardPanel.h"
#include "gui/workspace/WorkspacePanel.h"
#include "gui/Workspace.h"
#include "Globals.h"
#include "Settings.h"
#include "BinaryData.h"

namespace Element {

class WorkspacesContentComponent::Impl
{
public:
    Impl (WorkspacesContentComponent& o, AppController& a)
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

    void showPanel (const DockPanelInfo& info)
    {
        auto& dock = getDock();
        const auto panelId      = info.identifier;
        const auto singleton    = info.singleton;

        if (singleton)
        {
            kv::DockPanel* panel = nullptr;
            for (int i = 0; i < dock.getNumPanels(); ++i)
            {
                auto* p = dock.getPanel (i);
                if (p->getType() == panelId)
                    { panel = p; break; }
            }

            if (panel != nullptr)
            {
                dock.selectPanel (panel);
                return;
            }
        }
        
        if (DockPlacement::isValid (info.placement))
        {
            dock.createItem (panelId, static_cast<DockPlacement> (info.placement));
        }
        else
        {
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

    const DockPanelInfo* findPanelInfo (const Identifier& panelID) const
    {
        for (const auto* info : (*const_cast<Impl*>(this)).getDock().getPanelDescriptions())
            if (info->identifier == panelID)
                return info;
        return nullptr;
    }

    String getPanelID (CommandID command)
    {
        String result;
        switch (command)
        {
            case Commands::toggleVirtualKeyboard:
                result = PanelIDs::virtualKeyboard.toString();
                break;
            case Commands::showControllerDevices:
                result = PanelIDs::controllers.toString();
                break;
            case Commands::showKeymapEditor:
                result = PanelIDs::keymaps.toString();
                break;
            case Commands::showPluginManager:
                result = PanelIDs::pluginManager.toString();
                break;
            case Commands::showSessionConfig:
                result = PanelIDs::sessionSettings.toString();
                break;
            case Commands::showGraphConfig:
                result = PanelIDs::graphSettings.toString();
                break;

            case Commands::showPatchBay:
                result = PanelIDs::graphEditor.toString();
                break;

            case Commands::showGraphEditor:
                result = PanelIDs::graphEditor.toString();
                break;

            case Commands::showGraphMixer:
                result = PanelIDs::graphMixer.toString();
                break;

            case Commands::showConsole:
                result = PanelIDs::luaConsole.toString();
                break;

            case Commands::toggleChannelStrip:
                result = PanelIDs::nodeChannelStrip.toString();
                break;

            // noop commands
            case Commands::showLastContentView:
            case Commands::rotateContentView:
                break;

            default:
                result.clear();
                break;
        }
        return result;
    }

    AppController& app;
    WorkspacesContentComponent& owner;
    Workspace workspace;
};

WorkspacesContentComponent::WorkspacesContentComponent (AppController& controller)
    : ContentComponent (controller)
{
    impl.reset (new Impl (*this, controller));
}

WorkspacesContentComponent::~WorkspacesContentComponent() noexcept
{
    impl.reset (nullptr);
}

bool WorkspacesContentComponent::perform (const InvocationInfo& info)
{
    auto ID = impl->getPanelID (info.commandID);
    if (ID.isEmpty())
        return false;
    
    if (const auto* info = impl->findPanelInfo (ID))
    {
        impl->showPanel (*info);
        return true;
    }

    return false;
}

void WorkspacesContentComponent::resizeContent (const Rectangle<int>& area)
{
    impl->resized (area);
}

void WorkspacesContentComponent::stabilize (const bool refreshDataPathTrees)
{
    impl->stabilizePanels();
}

void WorkspacesContentComponent::stabilizeViews()
{
    impl->stabilizePanels();
}

String WorkspacesContentComponent::getWorkspaceName() const
{
    return impl->workspace.getName();
}

WorkspaceState WorkspacesContentComponent::getWorkspaceState()
{
    WorkspaceState state (impl->workspace);
    return state;
}

void WorkspacesContentComponent::applyWorkspaceState (const WorkspaceState& state)
{
    auto& workspace = impl->workspace;
    workspace.applyState (state);
}

#define EL_WORKSPACES_MENU_OFFSET   100000
void WorkspacesContentComponent::addWorkspaceItemsToMenu (PopupMenu& menu)
{
    auto& dock = impl->getDock();
    const int offset = EL_WORKSPACES_MENU_OFFSET;
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

void WorkspacesContentComponent::handleWorkspaceMenuResult (int result)
{
    const int index = result - EL_WORKSPACES_MENU_OFFSET;
    const auto& descs = impl->getDock().getPanelDescriptions();
    if (! isPositiveAndBelow (index, descs.size()))
        return;
    impl->showPanel (*descs[index]);
}

void WorkspacesContentComponent::saveState (PropertiesFile*)
{
    if (auto* props = getAppController().getWorld().getSettings().getUserSettings())
        if (auto* vk = getVirtualKeyboardView())
            vk->saveState (props);
}

void WorkspacesContentComponent::restoreState (PropertiesFile*)
{
    auto& settings = getAppController().getWorld().getSettings();
    if (auto* props = settings.getUserSettings())
        if (auto* vk = getVirtualKeyboardView())
            vk->restoreState (props);

    const auto stateName = settings.getWorkspace();
    WorkspaceState state = WorkspaceState::loadByFileOrName (stateName);
    if (! state.isValid())
        state = WorkspaceState::loadByName ("Classic");
    applyWorkspaceState (state);
}

void WorkspacesContentComponent::getSessionState (String&)
{
}

void WorkspacesContentComponent::applySessionState (const String&)
{
}

//==============================================================================
bool WorkspacesContentComponent::isVirtualKeyboardVisible() const
{
    if (auto* panel = impl->findPanel<VirtualKeyboardPanel>())
        return panel->isVisible();
    return false;
}

void WorkspacesContentComponent::setVirtualKeyboardVisible (const bool isVisible)
{
    ContentComponent::setVirtualKeyboardVisible (isVisible);
}

void WorkspacesContentComponent::toggleVirtualKeyboard()
{
    ContentComponent::toggleVirtualKeyboard();
}

VirtualKeyboardView* WorkspacesContentComponent::getVirtualKeyboardView() const
{
    if (auto* panel = impl->findPanel<VirtualKeyboardPanel>())
        return dynamic_cast<VirtualKeyboardView*> (&panel->getView());
    return nullptr;
}

} // namespace Element
