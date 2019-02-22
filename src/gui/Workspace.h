/*
    Workspace.h - This file is part of Element
    Copyright (C) 2016-2018 Kushview, LLC.  All rights reserved.
*/

#if EL_DOCKING

#pragma once

#include "ElementApp.h"

namespace Element {

class AppController;
class GuiController;
class Globals;

class Workspace : public Component
{
public:
    Workspace (Globals&, AppController&, GuiController&);
    virtual ~Workspace();

    void paint (Graphics& g) override;
    void mouseDown (const MouseEvent& ev) override;

    void resized() override;

    Dock& getDock();

private:
    Dock dock;
    Globals& world;
    AppController& app;
    GuiController& gui;
};

class WorkspaceWindow : public DocumentWindow
{
public:
    WorkspaceWindow() : DocumentWindow ("Workspace", Colours::black, DocumentWindow::allButtons, true)
    {
        setUsingNativeTitleBar (true);
        centreWithSize (getWidth(), getHeight());
    }
};

}

#endif
