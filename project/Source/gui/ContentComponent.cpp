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

#include "gui/ContentComponent.h"
#include "gui/SequencerComponent.h"
#include "gui/Workspace.h"

namespace Element {
namespace Gui {

ContentComponent::ContentComponent (GuiApp& app_)
    : gui (app_)
{
    setOpaque (true);
   #if 0
    addAndMakeVisible (display = new ScreenDisplay());
   #else
    addAndMakeVisible (seq = new SequencerComponent (gui));
   #endif
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
    g.fillAll (Colours::darkgrey);
}

void ContentComponent::resized()
{
    const Rectangle<int> r (getLocalBounds());
   #if 1
    seq->setBounds (r.reduced (2));
   #else
    display->setBounds (r.reduced (2));
   #endif
}

GuiApp& ContentComponent::app() { return gui; }

}}
