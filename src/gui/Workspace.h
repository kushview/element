/*
    Workspace.h - This file is part of Element
    Copyright (C) 2016-2018 Kushview, LLC.  All rights reserved.
*/

#if EL_DOCKING

#pragma once

#include "ElementApp.h"

namespace Element {

class Workspace :  public Component
{
public:
    Workspace();
    virtual ~Workspace();

    void setMainComponent (Component* c);

    void paint (Graphics& g) override;
    void mouseDown (const MouseEvent& ev) override;

    void resized() override;

    Dock& getDock();

private:
    Dock dock;
};

class WorkspaceWindow : public DocumentWindow
{
public:
    WorkspaceWindow() : DocumentWindow ("Workspace", Colours::black, DocumentWindow::allButtons, true)
    {
        setUsingNativeTitleBar (true);
        setContentOwned (new Workspace(), true);
        centreWithSize (getWidth(), getHeight());
    }
};

}

#endif
