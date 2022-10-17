/*
    DockArea.cpp - This file is part of Element
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

#include "gui/Dock.h"

namespace element {

DockArea::DockArea (const bool vertical)
    : layout (*this, vertical)
{
}

DockArea::DockArea (DockPlacement placement)
    : layout (*this)
{
    switch (placement)
    {
        case DockPlacement::Top:
        case DockPlacement::Bottom:
            layout.setVertical (false);
            break;
        case DockPlacement::Left:
        case DockPlacement::Right:
        default:
            layout.setVertical (true);
            break;
    }
}

DockArea::~DockArea()
{
}

void DockArea::append (DockArea* const area)
{
    insert (-1, area);
}

void DockArea::append (DockItem* const item)
{
    insert (-1, item);
}

Component* DockArea::getItem (const int index) const { return layout.getItems()[index]; }

void DockArea::insert (int index, Component* const item, Dock::SplitType split)
{
    if (auto* di = dynamic_cast<DockItem*> (item))
        insert (index, di, split);
    else if (auto* da = dynamic_cast<DockArea*> (item))
        insert (index, da, split);
    else
    {
        jassertfalse;
    } // this should only be used with dock areas and items
}

void DockArea::insert (int index, DockArea* const area, Dock::SplitType split)
{
    area->setVertical (! isVertical());
    layout.insert (index, area, split);
    addAndMakeVisible (area);
    resized();
}

void DockArea::insert (int index, DockItem* const item, Dock::SplitType split)
{
    layout.insert (index, item, split);
    addAndMakeVisible (item);
    item->repaint();
    resized();
}

void DockArea::moveItem (int source, int target)
{
    layout.move (source, target);
    resized();
}

void DockArea::remove (Component* const item)
{
    removeChildComponent (item);
    layout.remove (item);
}

void DockArea::remove (DockArea* const area)
{
    removeChildComponent (area);
    layout.remove (area);
}

void DockArea::remove (DockItem* const item)
{
    removeChildComponent (item);
    layout.remove (item);
}

void DockArea::setVertical (const bool vertical)
{
    layout.setVertical (vertical);
}

void DockArea::resized()
{
    layout.layoutItems (0, 0, getWidth(), getHeight());
}

ValueTree DockArea::getState() const
{
    ValueTree state ("area");
    state.setProperty ("bounds", getLocalBounds().toString(), nullptr)
        .setProperty ("vertical", isVertical(), nullptr)
        .setProperty ("barSize", layout.getBarSize(), nullptr)
        .setProperty ("sizes", layout.getSizesString(), nullptr);

    for (auto* const child : layout.getItems())
    {
        if (auto* const item = dynamic_cast<DockItem*> (child))
            state.addChild (item->getState(), -1, nullptr);
        else if (auto* const area = dynamic_cast<DockArea*> (child))
            state.addChild (area->getState(), -1, nullptr);
    }

    return state;
}

} // namespace element
