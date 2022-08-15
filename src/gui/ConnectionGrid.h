/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

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

#pragma once

#include "ElementApp.h"
#include "gui/ContentComponent.h"

namespace element {

class BreadCrumbComponent;
class Node;

class ConnectionGrid : public ContentView,
                       public DragAndDropTarget
{
public:
    ConnectionGrid();
    ~ConnectionGrid();

    void setNode (const Node& node);

    void didBecomeActive() override;

    void paint (Graphics&) override;
    void resized() override;

    void mouseDown (const MouseEvent& ev) override;

    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override;
    void itemDropped (const SourceDetails& dragSourceDetails) override;

#if 0
    void itemDragEnter (const SourceDetails& dragSourceDetails) override;
    void itemDragMove (const SourceDetails& dragSourceDetails) override;
    void itemDragExit (const SourceDetails& dragSourceDetails) override;
    bool shouldDrawDragImageWhenOver() override;
#endif

private:
    ScopedPointer<BreadCrumbComponent> breadcrumb;

    friend class PatchMatrix;
    class PatchMatrix;
    PatchMatrix* matrix;

    friend class Controls;
    class Controls;
    Controls* controls;

    friend class Sources;
    class Sources;
    Sources* sources;

    friend class Destinations;
    class Destinations;
    Destinations* destinations;

    friend class Quads;
    class Quads;
    ScopedPointer<QuadrantLayout> quads;
};

} // namespace element
