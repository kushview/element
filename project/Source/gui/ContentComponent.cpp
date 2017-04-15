/*
    ContentComponent.cpp - This file is part of Element
    Copyright (C) 2015  Kushview, LLC.  All rights reserved.

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

#include "gui/LookAndFeel.h"
#include "gui/ContentComponent.h"

namespace Element {

class ContentContainer : public Component
{
public:
    ContentContainer (GuiApp& gui)
    {
        addAndMakeVisible (dummy1);
        addAndMakeVisible (bar = new StretchableLayoutResizerBar (&layout, 1, false));
        addAndMakeVisible (dummy2 = new Component());
        updateLayout();
        resized();
    }
    
    virtual ~ContentContainer() { }
    
    void resized() override
    {
        Component* comps[] = { dummy1.get(), bar.get(), dummy2.get() };
        layout.layOutComponents (comps, 3, 0, 0, getWidth(), getHeight(), true, true);
    }
    
    void stabilize()
    {

    }
    
private:
    StretchableLayoutManager layout;
    ScopedPointer<StretchableLayoutResizerBar> bar;
    ScopedPointer<Component> dummy1, dummy2;
    
    void updateLayout()
    {
        layout.setItemLayout (0, 200, -1.0, 200);
        layout.setItemLayout (1, 4, 4, 4);
        layout.setItemLayout (2, 60, -1.0, 500);
    }
};

ContentComponent::ContentComponent (GuiApp& app_)
    : gui (app_)
{
    setOpaque (true);
    
    addAndMakeVisible (nav = new Component());
    addAndMakeVisible (bar1 = new StretchableLayoutResizerBar (&layout, 1, true));
    addAndMakeVisible (container = new ContentContainer (app_));
    
    updateLayout();
    resized();
}

ContentComponent::~ContentComponent()
{
    toolTips = nullptr;
}

void ContentComponent::childBoundsChanged (Component* child)
{
}

void ContentComponent::paint (Graphics &g)
{
    g.fillAll (LookAndFeel::backgroundColor);
}

void ContentComponent::resized()
{
    Component* comps[3] = { nav.get(), bar1.get(), container.get() };
    layout.layOutComponents (comps, 3, 0, 0, getWidth(), getHeight(), false, true);
}

void ContentComponent::setRackViewComponent (Component* comp)
{
    
}

void ContentComponent::setRackViewNode (GraphNodePtr node)
{
    
}

GuiApp& ContentComponent::app() { return gui; }

void ContentComponent::stabilize() { }

void ContentComponent::updateLayout()
{
    layout.setItemLayout (0, 60, 400, 200);
    layout.setItemLayout (1, 4, 4, 4);
    layout.setItemLayout (2, 0, -1.0, 500);
}

}


