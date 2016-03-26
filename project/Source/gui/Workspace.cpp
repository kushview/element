/*
    Workspace.cpp - This file is part of Element
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

#include "Workspace.h"

namespace Element {

Workspace::Workspace()
{
    addAndMakeVisible (dock = new Dock());
    setMainComponent (new ScreenDisplay());
    dock->getBottomArea().setVisible (false);
}

Workspace::~Workspace()
{
    dock = nullptr;
}

Dock& Workspace::getDock()
{
    jassert (dock != nullptr);
    return *dock;
}

void Workspace::setMainComponent (Component* c)
{
    DockItem* item = dock->createItem ("test", "Test Item", Dock::TopArea);
    item->setContentOwned (c);
    item->setMaximized (true);
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
    Rectangle<int> b (getLocalBounds());
    dock->setBounds (b.reduced (3));
}

}
