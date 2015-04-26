/*
    ContentComponent.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.

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
namespace Gui {

    class SessionRootTreeItem;
    class SessionTreePanel;
    class GuiApp;
    class Workspace;

    class ContentComponent :  public Component,
                              public DragAndDropContainer
    {
    public:

        ContentComponent (GuiApp& app);
        ~ContentComponent();

        void childBoundsChanged (Component* child);
        void paint (Graphics &g);
        void resized();

        GuiApp& app();

    private:

        GuiApp& gui;
        MidiKeyboardState keyboard;
        ScopedPointer<Workspace>     workspace;
        ScopedPointer<TooltipWindow> toolTips;

    };


}}  /* namespace Element::Gui */

#endif // ELEMENT_CONTENT_COMPONENT_H
