/*
    SessionTreePanel.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "gui/TreeviewBase.h"
#include "session/Session.h"

namespace Element {

class Session;
class GuiApp;

class SessionTreePanel : public TreePanelBase
{
public:
    explicit SessionTreePanel (GuiApp& gui);
    virtual ~SessionTreePanel();

    void mouseDown (const MouseEvent &event);

    Session& session();

private:

    GuiApp& gui;

};


class SessionMediaTreePanel :  public TreePanelBase
{

};

}
