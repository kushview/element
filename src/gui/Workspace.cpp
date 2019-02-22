/*
    Workspace.cpp - This file is part of Element
 */

#if EL_DOCKING

#include "controllers/AppController.h"
#include "gui/workspace/PanelTypes.h"
#include "gui/workspace/WorkspacePanel.h"
#include "gui/Workspace.h"

namespace Element {

Workspace::Workspace (Globals& w, AppController& a, GuiController& g)
    : world (w), app (a), gui (g)
{
    dock.registerPanelType (new GenericPanelType());
    dock.registerPanelType (new ApplicationPanelType());
    
    dock.onPanelAdded = [this](DockPanel* panel)
    {
        if (auto* const wp = dynamic_cast<WorkspacePanel*> (panel))
        {
            wp->initializeView (app);
            wp->didBecomeActive();
        }
    };

    auto* item = dock.createItem (PanelIDs::virtualKeyboard, DockPlacement::Top);
    dock.createItem (PanelIDs::graphMixer,      DockPlacement::Top);
    dock.createItem (PanelIDs::graphEditor,     DockPlacement::Bottom);
    dock.createItem (PanelIDs::nodeEditor,      DockPlacement::Left);
    dock.createItem (PanelIDs::nodeChannelStrip, DockPlacement::Right);
    dock.createItem (PanelIDs::nodeMidi,        DockPlacement::Right);
    dock.createItem (PanelIDs::plugins,         DockPlacement::Left);

    addAndMakeVisible (dock);
    setSize (1280, 640);
}

Workspace::~Workspace()
{
}

Dock& Workspace::getDock() { return dock; }

void Workspace::paint (Graphics& g)
{
    g.fillAll (Colours::red);
}

void Workspace::mouseDown (const MouseEvent& /*ev*/)
{

}

void Workspace::resized()
{
    dock.setBounds (getLocalBounds());
}

}

#endif
