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

#include "gui/Dock.h"
#include "gui/DockLayout.h"

namespace element {

DockLayoutManager::DockLayoutManager() {}
DockLayoutManager::~DockLayoutManager() {}

void DockLayoutManager::clearAllItems()
{
    items.clear();
    totalSize = 0;
}

void DockLayoutManager::setItemLayout (const int itemIndex, const double minimumSize, const double maximumSize, const double preferredSize)
{
    auto* layout = getInfoFor (itemIndex);

    if (layout == nullptr)
    {
        layout = new ItemLayoutProperties();
        layout->itemIndex = itemIndex;

        int i;
        for (i = 0; i < items.size(); ++i)
            if (items.getUnchecked (i)->itemIndex > itemIndex)
                break;

        items.insert (i, layout);
    }

    layout->minSize = minimumSize;
    layout->maxSize = maximumSize;
    layout->preferredSize = preferredSize;
    layout->currentSize = 0;
}

bool DockLayoutManager::getItemLayout (const int itemIndex, double& minimumSize, double& maximumSize, double& preferredSize) const
{
    if (auto* layout = getInfoFor (itemIndex))
    {
        minimumSize = layout->minSize;
        maximumSize = layout->maxSize;
        preferredSize = layout->preferredSize;
        return true;
    }

    return false;
}

void DockLayoutManager::setTotalSize (const int newTotalSize)
{
    totalSize = newTotalSize;

    fitComponentsIntoSpace (0, items.size(), totalSize, 0);
}

int DockLayoutManager::getItemCurrentPosition (const int itemIndex) const
{
    int pos = 0;
    for (int i = 0; i < itemIndex; ++i)
        if (auto* layout = getInfoFor (i))
            pos += layout->currentSize;
    return pos;
}

int DockLayoutManager::getItemCurrentAbsoluteSize (const int itemIndex) const
{
    if (auto* layout = getInfoFor (itemIndex))
        return layout->currentSize;
    return 0;
}

double DockLayoutManager::getItemCurrentRelativeSize (const int itemIndex) const
{
    if (auto* layout = getInfoFor (itemIndex))
        return -layout->currentSize / (double) totalSize;
    return 0;
}

void DockLayoutManager::setItemPosition (const int itemIndex, int newPosition)
{
    for (int i = items.size(); --i >= 0;)
    {
        auto* layout = items.getUnchecked (i);
        if (layout->itemIndex != itemIndex)
            continue;

        auto realTotalSize = jmax (totalSize, getMinimumSizeOfItems (0, items.size()));
        auto minSizeAfterThisComp = getMinimumSizeOfItems (i, items.size());
        auto maxSizeAfterThisComp = getMaximumSizeOfItems (i + 1, items.size());
        newPosition = jmax (newPosition, totalSize - maxSizeAfterThisComp - layout->currentSize);
        newPosition = jmin (newPosition, realTotalSize - minSizeAfterThisComp);

        if (! barMoving)
        {
            auto endPos = fitComponentsIntoSpace (0, i, newPosition, 0);
            endPos += layout->currentSize;
            fitComponentsIntoSpace (i + 1, items.size(), totalSize - endPos, endPos);
        }
        else
        {
            jassert (i > 0 && i % 2 != 0);

            const auto deltaPos = newPosition - getItemCurrentPosition (layout->itemIndex);
            const bool forward = deltaPos > 0;

            ItemLayoutProperties* leftOrTopItem = nullptr;
            for (int i = layout->itemIndex; --i >= 0;)
            {
                if (i % 2 != 0)
                    continue;
                jassert (i != layout->itemIndex);
                auto* info = getInfoFor (i);
                if (forward)
                {
                    leftOrTopItem = info;
                    break;
                }
                else
                {
                    if (info->currentSize > sizeToRealSize (info->minSize, totalSize))
                    {
                        leftOrTopItem = info;
                        break;
                    }
                }
            }

            ItemLayoutProperties* rightOrBottomItem = nullptr;
            for (int i = layout->itemIndex + 1; i < items.size(); ++i)
            {
                if (i % 2 != 0)
                    continue;
                jassert (i != layout->itemIndex);
                auto* info = getInfoFor (i);
                if (forward)
                {
                    if (info->currentSize > sizeToRealSize (info->minSize, totalSize))
                    {
                        rightOrBottomItem = info;
                        break;
                    }
                }
                else
                {
                    rightOrBottomItem = info;
                    break;
                }
            }

            if (leftOrTopItem)
            {
                leftOrTopItem->currentSize = jmax (sizeToRealSize (leftOrTopItem->minSize, totalSize),
                                                   leftOrTopItem->currentSize + deltaPos);
            }

            if (rightOrBottomItem)
            {
                if (forward)
                {
                    rightOrBottomItem->currentSize = jmax (sizeToRealSize (rightOrBottomItem->minSize, totalSize),
                                                           rightOrBottomItem->currentSize - deltaPos);
                }
                else
                {
                    if (leftOrTopItem)
                        rightOrBottomItem->currentSize =
                            jmax (sizeToRealSize (rightOrBottomItem->minSize, totalSize),
                                  rightOrBottomItem->currentSize - deltaPos);
                }
            }
        }

        updatePrefSizesToMatchCurrentPositions();

        break;
    }
}

void DockLayoutManager::layOutComponents (Component** const components, int numComponents, int x, int y, int w, int h, const bool vertically, const bool resizeOtherDimension)
{
    setTotalSize (vertically ? h : w);

    int pos = vertically ? y : x;

    for (int i = 0; i < numComponents; ++i)
    {
        if (auto* layout = getInfoFor (i))
        {
            if (auto* c = components[i])
            {
                if (i == numComponents - 1)
                {
                    // if it's the last item, crop it to exactly fit the available space..
                    if (resizeOtherDimension)
                    {
                        if (vertically)
                            c->setBounds (x, pos, w, jmax (layout->currentSize, h - pos));
                        else
                            c->setBounds (pos, y, jmax (layout->currentSize, w - pos), h);
                    }
                    else
                    {
                        if (vertically)
                            c->setBounds (c->getX(), pos, c->getWidth(), jmax (layout->currentSize, h - pos));
                        else
                            c->setBounds (pos, c->getY(), jmax (layout->currentSize, w - pos), c->getHeight());
                    }
                }
                else
                {
                    if (resizeOtherDimension)
                    {
                        if (vertically)
                            c->setBounds (x, pos, w, layout->currentSize);
                        else
                            c->setBounds (pos, y, layout->currentSize, h);
                    }
                    else
                    {
                        if (vertically)
                            c->setBounds (c->getX(), pos, c->getWidth(), layout->currentSize);
                        else
                            c->setBounds (pos, c->getY(), layout->currentSize, c->getHeight());
                    }
                }
            }

            pos += layout->currentSize;
        }
    }
}

DockLayoutManager::ItemLayoutProperties* DockLayoutManager::getInfoFor (const int itemIndex) const
{
    for (auto* i : items)
        if (i->itemIndex == itemIndex)
            return i;
    return nullptr;
}

int DockLayoutManager::fitComponentsIntoSpace (const int startIndex, const int endIndex, const int availableSpace, int startPos)
{
    // calculate the total sizes
    double totalIdealSize = 0.0;
    int totalMinimums = 0;

    for (int i = startIndex; i < endIndex; ++i)
    {
        auto* layout = items.getUnchecked (i);

        layout->currentSize = sizeToRealSize (layout->minSize, totalSize);

        totalMinimums += layout->currentSize;
        totalIdealSize += sizeToRealSize (layout->preferredSize, totalSize);
    }

    if (totalIdealSize <= 0)
        totalIdealSize = 1.0;

    // now calc the best sizes..
    int extraSpace = availableSpace - totalMinimums;

    while (extraSpace > 0)
    {
        int numWantingMoreSpace = 0;
        int numHavingTakenExtraSpace = 0;

        // first figure out how many comps want a slice of the extra space..
        for (int i = startIndex; i < endIndex; ++i)
        {
            auto* layout = items.getUnchecked (i);

            auto sizeWanted = sizeToRealSize (layout->preferredSize, totalSize);

            auto bestSize = jlimit (layout->currentSize,
                                    jmax (layout->currentSize, sizeToRealSize (layout->maxSize, totalSize)),
                                    roundToInt (sizeWanted * availableSpace / totalIdealSize));

            if (bestSize > layout->currentSize)
                ++numWantingMoreSpace;
        }

        // ..share out the extra space..
        for (int i = startIndex; i < endIndex; ++i)
        {
            auto* layout = items.getUnchecked (i);

            auto sizeWanted = sizeToRealSize (layout->preferredSize, totalSize);

            auto bestSize = jlimit (layout->currentSize,
                                    jmax (layout->currentSize, sizeToRealSize (layout->maxSize, totalSize)),
                                    roundToInt (sizeWanted * availableSpace / totalIdealSize));

            auto extraWanted = bestSize - layout->currentSize;

            if (extraWanted > 0)
            {
                auto extraAllowed = jmin (extraWanted,
                                          extraSpace / jmax (1, numWantingMoreSpace));

                if (extraAllowed > 0)
                {
                    ++numHavingTakenExtraSpace;
                    --numWantingMoreSpace;

                    layout->currentSize += extraAllowed;
                    extraSpace -= extraAllowed;
                }
            }
        }

        if (numHavingTakenExtraSpace <= 0)
            break;
    }

    // ..and calculate the end position
    for (int i = startIndex; i < endIndex; ++i)
    {
        auto* layout = items.getUnchecked (i);
        startPos += layout->currentSize;
    }

    return startPos;
}

int DockLayoutManager::getCurrentSizeOfItems (const int startIndex,
                                              const int endIndex) const
{
    int totalCurrent = 0;

    for (int i = startIndex; i < endIndex; ++i)
        totalCurrent += items.getUnchecked (i)->currentSize;

    return totalCurrent;
}

int DockLayoutManager::getMinimumSizeOfItems (const int startIndex,
                                              const int endIndex) const
{
    int totalMinimums = 0;

    for (int i = startIndex; i < endIndex; ++i)
        totalMinimums += sizeToRealSize (items.getUnchecked (i)->minSize, totalSize);

    return totalMinimums;
}

int DockLayoutManager::getMaximumSizeOfItems (const int startIndex, const int endIndex) const
{
    int totalMaximums = 0;

    for (int i = startIndex; i < endIndex; ++i)
        totalMaximums += sizeToRealSize (items.getUnchecked (i)->maxSize, totalSize);

    return totalMaximums;
}

void DockLayoutManager::updatePrefSizesToMatchCurrentPositions()
{
    for (int i = 0; i < items.size(); ++i)
    {
        auto* layout = items.getUnchecked (i);

        layout->preferredSize = (layout->preferredSize < 0) ? getItemCurrentRelativeSize (i)
                                                            : getItemCurrentAbsoluteSize (i);
    }
}

int DockLayoutManager::sizeToRealSize (double size, int totalSpace)
{
    if (size < 0)
        size *= -totalSpace;

    return roundToInt (size);
}

DockLayoutResizerBar::DockLayoutResizerBar (DockLayoutManager* layout_,
                                            const int index,
                                            const bool vertical)
    : layout (layout_), itemIndex (index), isVertical (vertical)
{
    setRepaintsOnMouseActivity (false);
    setMouseCursor (vertical ? MouseCursor::LeftRightResizeCursor
                             : MouseCursor::UpDownResizeCursor);
}

DockLayoutResizerBar::~DockLayoutResizerBar()
{
}

void DockLayoutResizerBar::paint (Graphics& g) {}

void DockLayoutResizerBar::mouseDown (const MouseEvent&)
{
    mouseDownPos = layout->getItemCurrentPosition (itemIndex);
    layout->barMoving = true;
}

void DockLayoutResizerBar::mouseDrag (const MouseEvent& e)
{
    const int desiredPos = mouseDownPos + (isVertical ? e.getDistanceFromDragStartX() : e.getDistanceFromDragStartY());

    if (layout->getItemCurrentPosition (itemIndex) != desiredPos)
    {
        layout->setItemPosition (itemIndex, desiredPos);
        hasBeenMoved();
    }
}

void DockLayoutResizerBar::mouseUp (const MouseEvent&)
{
    layout->barMoving = false;
}

void DockLayoutResizerBar::hasBeenMoved()
{
    if (Component* parent = getParentComponent())
        parent->resized();
}

DockLayout::DockLayout (Component& holder_, bool vertical_)
    : holder (holder_), vertical (vertical_)
{
}

DockLayout::~DockLayout() noexcept
{
    clear();
}

void DockLayout::clear()
{
    items.clear();
    comps.clear();
    bars.clear();
    layout.clearAllItems();
}

void DockLayout::append (Component* item)
{
    if (! items.contains (item))
    {
        if (items.size() > 0)
        {
            int index = comps.size();
            bars.add (new DockLayoutResizerBar (&layout, index, ! vertical));
            comps.add (bars.getLast());
            holder.addAndMakeVisible (bars.getLast());
            layout.setItemLayout (index, barSize, barSize, barSize);
        }

        const auto prefSize = jmax (100, vertical ? item->getHeight() : item->getWidth());
        layout.setItemLayout (comps.size(), 30, -1.0, prefSize);
        comps.add (item);
        items.add (item);
    }
}

void DockLayout::insert (int index, Component* const item, int splitType)
{
    if (items.contains (item))
        return;
    if (index >= items.size() || index < 0)
        index = -1;

    items.insert (index, item);

    // DBG("Split: " << Dock::getSplitString (splitType));

    if (splitType == Dock::SplitBefore || splitType == Dock::SplitAfter)
    {
        const auto offset = splitType == Dock::SplitBefore ? -1 : 1;
        if (auto* affectedItem = items[index + offset])
        {
            const int splitSize = (vertical ? affectedItem->getHeight() : affectedItem->getWidth()) / 2;
            if (vertical)
            {
                item->setSize (item->getWidth(), splitSize);
                affectedItem->setSize (affectedItem->getWidth(), splitSize);
            }
            else
            {
                item->setSize (splitSize, item->getHeight());
                affectedItem->setSize (splitSize, item->getHeight());
            }
        }
    }
    else
    {
        int itemSize = vertical ? item->getHeight() : item->getWidth();
        if (vertical)
        {
            item->setSize (holder.getWidth(), itemSize);
        }
        else
        {
            item->setSize (itemSize, holder.getHeight());
        }
    }

    buildComponentArray();
}

void DockLayout::remove (Component* const child)
{
    bool wasRemoved = true;

    if (items.contains (child))
    {
        items.removeFirstMatchingValue (child);
        wasRemoved = true;
    }

    if (wasRemoved)
        buildComponentArray();
}

String DockLayout::getSizesString() const
{
    StringArray sizes;

    for (int i = 0; i < comps.size(); ++i)
    {
        double minSize, maxSize, prefSize;
        layout.getItemLayout (i, minSize, maxSize, prefSize);
        sizes.add (String (minSize));
        sizes.add (String (maxSize));
        sizes.add (String (prefSize));
    }

    return sizes.joinIntoString (":");
}

void DockLayout::setSizes (const String& strSizes)
{
    StringArray sizes = StringArray::fromTokens (strSizes, ":", "'");
    if (sizes.size() < 3 || sizes.size() % 3 != 0)
    {
        jassertfalse;
        return;
    }

    layout.clearAllItems();
    for (int i = 0, j = 0; i < sizes.size(); i += 3, ++j)
    {
        layout.setItemLayout (j,
                              sizes.getReference (i).getDoubleValue(),
                              sizes.getReference (i + 1).getDoubleValue(),
                              sizes.getReference (i + 2).getDoubleValue());
    }

    holder.resized();
}

void DockLayout::layoutItems (int x, int y, int w, int h)
{
    if (comps.size() > 0)
    {
        layout.layOutComponents ((Component**) &comps.getReference (0),
                                 comps.size(),
                                 x,
                                 y,
                                 w,
                                 h,
                                 vertical,
                                 true);
    }
}

void DockLayout::layoutItems()
{
    layoutItems (0, 0, holder.getWidth(), holder.getHeight());
}

void DockLayout::move (int sourceIdx, int targetIdx)
{
    jassert (isPositiveAndBelow (sourceIdx, items.size()));
    jassert (isPositiveAndBelow (targetIdx, items.size()));
    auto* source = items[sourceIdx];
    auto* target = items[targetIdx];
    if (source && target)
    {
        jassert (comps.contains (source) && comps.contains (target));
        items.move (sourceIdx, targetIdx);
        buildComponentArray();
    }
}

void DockLayout::buildComponentArray()
{
    bars.clearQuick (true);
    comps.clearQuick();
    layout.clearAllItems();
    for (int i = 0; i < items.size(); ++i)
    {
        auto itemSize = vertical ? items[i]->getHeight() : items[i]->getWidth();
        layout.setItemLayout (comps.size(), 30, -1.0, itemSize);
        comps.add (items[i]);

        if (i != items.size() - 1)
        {
            int index = comps.size();
            bars.add (new DockLayoutResizerBar (&layout, index, ! vertical));
            comps.add (bars.getLast());
            holder.addAndMakeVisible (bars.getLast());
            layout.setItemLayout (index, barSize, barSize, barSize);
        }
    }

    holder.resized();
}

void DockLayout::setBarSize (int newBarSize)
{
    if (newBarSize != barSize)
    {
        barSize = newBarSize;
        buildComponentArray();
    }
}

} // namespace element
