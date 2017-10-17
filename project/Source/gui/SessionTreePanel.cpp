/*
    SessionTreePanel.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#include "gui/GuiCommon.h"
#include "gui/ContentComponent.h"
#include "gui/SessionTreePanel.h"
#include "gui/ViewHelpers.h"
#include "session/Node.h"

namespace Element {

SessionGraphsListBox::SessionGraphsListBox (Session* s)
    : session (nullptr)
{
    setModel (this);
    setSession (s);
}
    
SessionGraphsListBox::~SessionGraphsListBox()
{
    setModel(nullptr);
    session = nullptr;
}

int SessionGraphsListBox::getNumRows()
{
    return (session) ? session->getNumGraphs() : 0;
}
    
void SessionGraphsListBox::paintListBoxItem (int rowNumber, Graphics& g, int width, int height,
                                             bool rowIsSelected)
{
    if (! session)
        return;
    const Node node (session->getGraph (rowNumber));
    ViewHelpers::drawBasicTextRow ("  " + node.getName(), g, width, height, rowIsSelected);
}


SessionTreePanel::SessionTreePanel()
    : TreePanelBase ("session")
{
    tree.setInterceptsMouseClicks (false, true);
}

SessionTreePanel::~SessionTreePanel()
{
    setRoot (nullptr);
}

void SessionTreePanel::mouseDown (const MouseEvent &ev)
{
}

SessionPtr SessionTreePanel::getSession() const
{
    return session;
}

}
