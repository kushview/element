/*
    This file is part of the Kushview Modules for JUCE
    Copyright (c) 2014-2019  Kushview, LLC.  All rights reserved.

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

#include <element/juce.hpp>
#include "gui/DockPlacement.h"

namespace element {

class Dock;
class DockArea;
class DockItem;

class DockContainer : public Component
{
public:
    /** Constructor */
    DockContainer (Dock&);

    /** Destructor */
    ~DockContainer();

    /** Dock an item at the root level */
    bool dockItem (DockItem* const item, DockPlacement placement);

    /** Returns the root area */
    DockArea* getRootArea() const;

    bool contains (Component* comp);

    /** @internal */
    void resized() override;
    /** @internal */
    void paint (Graphics&) override;

private:
    friend class Dock;
    friend class DockWindow;
    Dock& dock;
    Component::SafePointer<DockArea> root;
    struct DropZone;
    OwnedArray<DropZone> zones;
};

} // namespace element
