/*
    This file is part of the Kushview Modules for JUCE
    Copyright (C) 2014-2019  Kushview, LLC.  All rights reserved.

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
#include "gui/DockPanel.h"
#include "gui/DockWindow.h"

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wunused-variable")

// #define PANEL_DBG(msg)      DBG(msg)
#define PANEL_DBG(msg)

namespace element {

class ScopedDockWindowCloser
{
public:
    ScopedDockWindowCloser (DockWindow* w)
        : window (w)
    {
    }

    ~ScopedDockWindowCloser()
    {
        if (auto* w = window.getComponent())
            if (w->empty())
                w->closeButtonPressed();
    }

private:
    Component::SafePointer<DockWindow> window;
};

/** This will reparent the single item in a dock area contained in a parent
    dock area */
static void maybeFlipLastItem (DockPanel* panel, DockArea* sourceArea)
{
    if (sourceArea != nullptr && sourceArea->getNumItems() == 1)
    {
        PANEL_DBG (panel->getName() << ": parent area items: " << sourceArea->getNumItems());
        if (auto* parentArea = sourceArea->getParentArea())
        {
            PANEL_DBG (panel->getName() << ": flip last item");
            PANEL_DBG (panel->getName() << ": source items: " << sourceArea->getNumItems());
            PANEL_DBG (panel->getName() << ": parent items: " << parentArea->getNumItems());
            PANEL_DBG (panel->getName() << ": source vert : " << (int) sourceArea->isVertical());
            PANEL_DBG (panel->getName() << ": parent vert : " << (int) parentArea->isVertical());
            auto* itemRef = sourceArea->getItem (0);
            const auto areaIdx = parentArea->indexOf (sourceArea);
            PANEL_DBG (panel->getName() << ": area index  : " << areaIdx);
            const auto sizes = parentArea->getSizesString();
            sourceArea->remove (itemRef);
            parentArea->remove (sourceArea);
            parentArea->insert (areaIdx, itemRef);
            parentArea->setSizes (sizes);
        }
        else
        {
            PANEL_DBG (panel->getName() << ": source area has no parent");
        }
    }
}

DockPanel::DockPanel() {}
DockPanel::~DockPanel() {}

void DockPanel::dockTo (DockItem* const target, DockPlacement placement)
{
    if (placement.isFloating() || target == nullptr)
        return;

    ScopedDockWindowCloser windowCloser (findParentComponentOfClass<DockWindow>());

    auto* const source = findParentComponentOfClass<DockItem>();
    auto* const sourceItem = source;
    auto* const sourceArea = sourceItem->getParentArea();
    auto* const targetItem = target;
    auto* const targetArea = targetItem->getParentArea();
    auto* const targetPanel = targetItem->getCurrentPanel();

    Dock& dock (targetItem->dock);

    if (sourceItem != nullptr && sourceItem == targetItem && targetItem->getNumPanels() == 1)
    {
        // docking to self with no other panels. noop
        return;
    }

    if (placement == DockPlacement::Center)
    {
        if (sourceArea != nullptr)
        {
            //noop
        }

        if (sourceItem != nullptr)
        {
            PANEL_DBG (getName() << ": detaching panel from source");
            sourceItem->detach (this);

            if (sourceItem->getNumPanels() <= 0)
            {
                PANEL_DBG (getName() << ": source now empty");
            }

            if (sourceArea != targetArea)
            {
                maybeFlipLastItem (this, sourceArea);
            }
        }

        targetItem->panels.add (this);
        targetItem->refreshPanelContainer (this);

        if (sourceArea != nullptr)
        {
            //noop
        }

        dock.triggerAsyncUpdate();
        return;
    }

    PANEL_DBG ("Docking Panel: " << getName() << " to " << Dock::getDirectionString (placement)
                                 << " of Item: " << targetPanel->getName());

    if (nullptr == targetArea)
    {
        jassertfalse; // need an area to dock to
        return;
    }

    if (placement.isVertical() == targetArea->isVertical()
        && (sourceItem != targetItem || sourceItem->getNumPanels() > 1))
    {
        // Same direction as target parent area
        // Not same item unless source has 2 or more panels
        PANEL_DBG (getName() << ": area is same direction");
        int offsetIdx = 0;

        const bool inSameArea = sourceArea == targetArea;

        PANEL_DBG (getName() << ": same area " << (int) inSameArea);

        int srcIdx = sourceArea->indexOf (sourceItem);
        int tgtIdx = targetArea->indexOf (targetItem);

        const Dock::SplitType split = inSameArea && sourceItem->getNumPanels() == 1
                                          ? Dock::NoSplit
                                          : Dock::getSplitType (placement);

        if (sourceItem->getNumPanels() == 1)
        {
            PANEL_DBG (getName() << ": source item single panel");
            if (inSameArea)
            {
                // same area, so just move the item
                if (tgtIdx < srcIdx)
                {
                    if (placement.isVertical())
                    {
                        if (placement == DockPlacement::Bottom)
                            ++offsetIdx;
                    }
                    else
                    {
                        if (placement == DockPlacement::Right)
                            ++offsetIdx;
                    }
                }
                else if (tgtIdx > srcIdx)
                {
                    if (placement.isVertical())
                    {
                        if (placement == DockPlacement::Top)
                            --offsetIdx;
                    }
                    else
                    {
                        if (placement == DockPlacement::Left)
                            --offsetIdx;
                    }
                }

                PANEL_DBG (getName() << ": move " << srcIdx << " to " << tgtIdx);
                targetArea->moveItem (srcIdx, tgtIdx + offsetIdx);
            }
            else
            {
                // not in same area, need to detach from source and flip if needed
                sourceItem->detach();
                maybeFlipLastItem (this, sourceArea);

                if (placement.isVertical())
                {
                    if (placement == DockPlacement::Bottom)
                        ++offsetIdx;
                }
                else
                {
                    if (placement == DockPlacement::Right)
                        ++offsetIdx;
                }

                int insertIdx = targetArea->indexOf (target) + offsetIdx;
                targetArea->insert (insertIdx, source, split);
            }
        }
        else if (source->getNumPanels() > 1)
        {
            PANEL_DBG (getName() << ": source item panels: " << source->getNumPanels());
            sourceItem->detach (this);
            maybeFlipLastItem (this, sourceArea);

            if (placement.isVertical())
            {
                if (placement == DockPlacement::Bottom)
                    ++offsetIdx;
            }
            else
            {
                if (placement == DockPlacement::Right)
                    ++offsetIdx;
            }

            int insertIdx = offsetIdx + targetArea->indexOf (target);
            targetArea->insert (insertIdx, dock.getOrCreateItem (this), split);
        }
    }
    else if (placement.isVertical() != targetArea->isVertical())
    {
        PANEL_DBG (getName() << ": area is opposite direction");
        // opposite direction as parent
        // Create a new area, flip orientation, add target item, and insert source item.
        const int insertAreaIdx = targetArea->indexOf (target);
        const auto sizes = targetArea->getSizesString();

        DockArea* newArea = dock.getOrCreateArea (placement.isVertical());
        newArea->setSize (targetItem->getWidth(), targetItem->getHeight());
        targetItem->detach();
        newArea->append (targetItem);

        const int insertPanelIdx = placement == DockPlacement::Left || placement == DockPlacement::Top ? 0 : -1;

        if (sourceItem->getNumPanels() == 1)
        {
            sourceItem->detach();
            newArea->insert (insertPanelIdx, source);
        }
        else if (sourceItem->getNumPanels() > 1)
        {
            sourceItem->detach (this);
            newArea->insert (insertPanelIdx, dock.getOrCreateItem (this));
        }
        else
        {
            newArea = nullptr;
            jassertfalse; // unhandled docking condition or target lost?
        }

        if (newArea != nullptr)
        {
            newArea->resized();
            targetArea->insert (insertAreaIdx, newArea, Dock::NoSplit);
            jassert (targetArea->getParentComponent() != nullptr);
            targetArea->setSizes (sizes);
        }
    }
    else
    {
        // unhandled docking condition
        jassertfalse;
    }

    dock.triggerAsyncUpdate();
}

void DockPanel::close()
{
    ScopedDockWindowCloser windowCloser (findParentComponentOfClass<DockWindow>());
    if (auto* const dock = findParentComponentOfClass<Dock>())
    {
        dock->removePanel (this);
    }
    else if (auto* parentItem = findParentComponentOfClass<DockItem>())
    {
        auto* const itemArea = parentItem->getParentArea();
        parentItem->detach (this);

        if (itemArea != nullptr)
        {
            auto* const parentArea = itemArea->getParentArea();
            if (parentArea && itemArea->getNumItems() <= 0)
                parentArea->remove (itemArea);
        }
    }
    else
    {
        jassertfalse;
    }
}

void DockPanel::undock()
{
    auto* const dock = findParentComponentOfClass<Dock>();
    if (! dock)
    {
        jassertfalse; // can't undock without the Dock
        return;
    }

    dock->undockPanel (this);
}

ValueTree DockPanel::getState() const
{
    ValueTree state ("panel");
    state.setProperty ("name", getName(), nullptr)
        .setProperty ("type", getTypeString(), nullptr)
        .setProperty ("bounds", getLocalBounds().toString(), nullptr);
    return state;
}

void DockPanel::paint (Graphics& g)
{
    g.setColour (Colours::white);
    g.drawText (getName(), 0, 0, getWidth(), getHeight(), Justification::centred);
}

void DockPanel::resized()
{
}

} // namespace element

JUCE_END_IGNORE_WARNINGS_GCC_LIKE
