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

#include "gui/DockContainer.h"
#include "gui/Dock.h"

namespace element {

namespace DockHelpers {

    static bool findComponentRecursive (Component* container, Component* object)
    {
        for (int i = 0; i < container->getNumChildComponents(); ++i)
        {
            auto* child = container->getChildComponent (i);
            if (object == child)
                return true;
            if (findComponentRecursive (child, object))
                return true;
        }

        return false;
    }

} // namespace DockHelpers

struct DockContainer::DropZone : public Component,
                                 public DragAndDropTarget
{
    DropZone (DockContainer& c, DockPlacement p)
        : container (c), placement (p)
    {
        jassert (placement.isDirectional());
        setInterceptsMouseClicks (false, false);
    }

    void paint (Graphics& g) override
    {
        if (! dragging)
            return;
        g.setOpacity (0.4);
        g.fillAll (Colours::greenyellow);
    }

    bool isInterestedInDragSource (const SourceDetails& details) override
    {
        return details.description.toString() == "DockPanel";
    }

    void itemDropped (const SourceDetails& details) override
    {
        DBG ("dropped");
    }

    void itemDragEnter (const SourceDetails&) override
    {
        dragging = true;
        repaint();
    }

    void itemDragMove (const SourceDetails&) override {}

    void itemDragExit (const SourceDetails&) override
    {
        dragging = false;
        repaint();
    }

    bool shouldDrawDragImageWhenOver() override { return false; }

    DockContainer& container;
    const DockPlacement placement;
    bool dragging = false;
};

DockContainer::DockContainer (Dock& d)
    : dock (d)
{
    for (int i = DockPlacement::Top; i <= DockPlacement::Right; ++i)
        addAndMakeVisible (zones.add (new DropZone (*this, i)), 10000 + i);
    root = dock.getOrCreateArea();
    addAndMakeVisible (root.getComponent(), 1);
}

DockContainer::~DockContainer()
{
    zones.clear (true);
    root = nullptr;
}

DockArea* DockContainer::getRootArea() const
{
    jassert (root != nullptr);
    return root.getComponent();
}

bool DockContainer::contains (Component* object)
{
    if (object == this || (root != nullptr && object == root))
        return true;
    return DockHelpers::findComponentRecursive (this, object);
}

bool DockContainer::dockItem (DockItem* const item, DockPlacement placement)
{
    if (! placement.isDirectional())
        return false;

    bool result = true;
    const int insertIdx = placement == DockPlacement::Top || placement == DockPlacement::Left ? 0 : -1;
    const auto split = insertIdx < 0 ? Dock::SplitBefore : Dock::SplitAfter;

    if (root->isVertical() == placement.isVertical())
    {
        root->insert (insertIdx, item, split);
    }
    else
    {
        DockArea* oldRoot = root.getComponent();
        jassert (oldRoot);
        removeChildComponent (oldRoot);
        root = dock.getOrCreateArea (! oldRoot->isVertical());
        jassert (root);
        addAndMakeVisible (root, 1);
        root->append (oldRoot);
        root->insert (insertIdx, item, split);
    }

    resized();

    return result;
}

void DockContainer::paint (Graphics& g) {}

void DockContainer::resized()
{
    auto r = getLocalBounds();
    root->setBounds (r);
    for (auto* zone : zones)
    {
        auto r2 = r;
        switch (zone->placement)
        {
            case DockPlacement::Top:
                zone->setBounds (r2.removeFromTop (20).reduced (10, 0));
                break;
            case DockPlacement::Left:
                zone->setBounds (r2.removeFromLeft (20).reduced (0, 10));
                break;
            case DockPlacement::Bottom:
                zone->setBounds (r2.removeFromBottom (20).reduced (10, 0));
                break;
            case DockPlacement::Right:
                zone->setBounds (r2.removeFromRight (20).reduced (0, 10));
                break;
        }
    }
}

} // namespace element
