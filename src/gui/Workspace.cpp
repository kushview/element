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

    auto* item = dock.createItem (PanelIDs::virtualKeyboard.toString(), DockPlacement::Top);
    
    dock.createItem (PanelIDs::graphMixer.toString(), DockPlacement::Top);
    dock.createItem (PanelIDs::graphEditor.toString(), DockPlacement::Bottom);
    dock.createItem (PanelIDs::nodeEditor.toString(), DockPlacement::Left);
    dock.createItem (PanelIDs::nodeChannelStrip.toString(), DockPlacement::Right);

    addAndMakeVisible (dock);
    setSize (1280, 640);
}

Workspace::~Workspace()
{
}

Dock& Workspace::getDock() { return dock; }

void Workspace::setMainComponent (Component* c)
{
}

void Workspace::paint (Graphics& g)
{
    g.fillAll (Colours::black);
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
