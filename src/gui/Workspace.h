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

    void paint (Graphics& g);
    void mouseDown (const MouseEvent& ev);

    void resized();

    Dock& getDock();

private:
    ScopedPointer<Dock> dock;
};

}

#endif
