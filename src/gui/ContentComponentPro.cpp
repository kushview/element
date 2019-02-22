/*
    ContentComponentPro.cpp - This file is part of Element
    Copyright (C) 2015-2019  Kushview, LLC.  All rights reserved.
*/

#if EL_DOCKING

#include "controllers/AppController.h"
#include "controllers/GuiController.h"
#include "gui/ContentComponentPro.h"
#include "gui/workspace/WorkspacePanel.h"
#include "gui/Workspace.h"
#include "Globals.h"

namespace Element {

class ContentComponentPro::Impl
{
public:
    Impl (ContentComponentPro& o, AppController& a)
        : app (a), owner (o),
          workspace (a.getGlobals(), a, *a.findChild<GuiController>())
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
}

#endif
