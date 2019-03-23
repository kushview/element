/*
    ContentComponentPro.h - This file is part of Element
    Copyright (c) 2016-2019 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "gui/ContentComponent.h"
#include "gui/Workspace.h"

namespace Element {

class AppController;

class ContentComponentPro :  public ContentComponent
{
public:
    ContentComponentPro (AppController& app);
    ~ContentComponentPro() noexcept;

    void resizeContent (const Rectangle<int>& area) override;

    //=========================================================================
    void stabilize (const bool refreshDataPathTrees = false) override;
    void stabilizeViews() override;

    //=========================================================================
    String getWorkspaceName() const override;
    WorkspaceState getWorkspaceState() override;
    void applyWorkspaceState (const WorkspaceState&) override;
    void addWorkspaceItemsToMenu (PopupMenu&) override;
    void handleWorkspaceMenuResult (int) override;

    //=========================================================================
    void saveState (PropertiesFile*) override;
    void restoreState (PropertiesFile*) override;
    void getState (String&) override;
    void setState (const String&) override;

private:
    class Impl; std::unique_ptr<Impl> impl;
};

}
