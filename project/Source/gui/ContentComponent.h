/*
    ContentComponent.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.

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

#ifndef ELEMENT_CONTENT_COMPONENT_H
#define ELEMENT_CONTENT_COMPONENT_H

#include "element/Juce.h"

namespace Element {

class ContentContainer;
class GraphEditorView;
class TransportBar;
class GuiApp;
class RackView;
class SequencerComponent;
class Workspace;

class ContentComponent :  public Component,
                          public DragAndDropContainer,
                          private Timer
{
public:
    ContentComponent (GuiApp& app);
    ~ContentComponent();

    void childBoundsChanged (Component* child) override;
    void paint (Graphics &g) override;
    void resized() override;

    void setRackViewComponent (Component* comp);
    void stabilize();
    
    GuiApp& app();

private:
    GuiApp& gui;
    ScopedPointer<ContentContainer> top;
    ScopedPointer<ScreenDisplay> display;
    ScopedPointer<SequencerComponent> seq;
    ScopedPointer<TransportBar>  transport;
    ScopedPointer<TooltipWindow> toolTips;
    ScopedPointer<GraphEditorView> graph;
    StretchableLayoutManager layoutVertical;
    ScopedPointer<StretchableLayoutResizerBar> bar1;
    ScopedPointer<RackView> rack;
    Shared<Monitor> playbackMonitor;
    
    friend class Timer;
    void timerCallback() override;
};

}

#endif // ELEMENT_CONTENT_COMPONENT_H
