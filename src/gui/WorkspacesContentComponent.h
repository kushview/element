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

#pragma once

#include "gui/ContentComponent.h"
#include "gui/Workspace.h"

namespace Element {

class AppController;

class WorkspacesContentComponent : public ContentComponent
{
public:
    WorkspacesContentComponent (AppController& app);
    ~WorkspacesContentComponent() noexcept;

    void resizeContent (const Rectangle<int>& area) override;

    //=========================================================================
    void stabilize (const bool refreshDataPathTrees = false) override;
    void stabilizeViews() override;

    //=========================================================================
    String getWorkspaceName() const;
    WorkspaceState getWorkspaceState();
    void applyWorkspaceState (const WorkspaceState&);
    void addWorkspaceItemsToMenu (PopupMenu&);
    void handleWorkspaceMenuResult (int);

    //=========================================================================
    void saveState (PropertiesFile*) override;
    void restoreState (PropertiesFile*) override;
    void getSessionState (String&) override;
    void applySessionState (const String&) override;

    //=========================================================================
    void getAllCommands (Array<CommandID>&) override;
    void getCommandInfo (CommandID, ApplicationCommandInfo&) override;
    bool perform (const InvocationInfo& info) override;

    //=========================================================================
    bool isVirtualKeyboardVisible() const override;
    void setVirtualKeyboardVisible (const bool isVisible) override;
    void toggleVirtualKeyboard() override;
    VirtualKeyboardView* getVirtualKeyboardView() const override;

    void setMainView (ContentView* v) override;
private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace Element
