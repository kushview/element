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

#include "JuceHeader.h"

namespace element {

class JUCE_API DockLayoutManager
{
public:
    DockLayoutManager();
    ~DockLayoutManager();

    void setItemLayout (int itemIndex, double minimumSize, double maximumSize, double preferredSize);
    bool getItemLayout (int itemIndex, double& minimumSize, double& maximumSize, double& preferredSize) const;
    void clearAllItems();
    void layOutComponents (Component** components, int numComponents, int x, int y, int width, int height, bool vertically, bool resizeOtherDimension);
    int getItemCurrentPosition (int itemIndex) const;
    int getItemCurrentAbsoluteSize (int itemIndex) const;
    double getItemCurrentRelativeSize (int itemIndex) const;
    void setItemPosition (int itemIndex, int newPosition);

private:
    friend class DockLayoutResizerBar;
    struct ItemLayoutProperties
    {
        int itemIndex;
        int currentSize;
        double minSize, maxSize, preferredSize;
    };

    OwnedArray<ItemLayoutProperties> items;
    int totalSize = 0;
    bool barMoving = false;

    static int sizeToRealSize (double size, int totalSpace);
    ItemLayoutProperties* getInfoFor (int itemIndex) const;
    void setTotalSize (int newTotalSize);
    int fitComponentsIntoSpace (int startIndex, int endIndex, int availableSpace, int startPos);
    int getCurrentSizeOfItems (int startIndex, int endIndex) const;
    int getMinimumSizeOfItems (int startIndex, int endIndex) const;
    int getMaximumSizeOfItems (int startIndex, int endIndex) const;
    void updatePrefSizesToMatchCurrentPositions();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DockLayoutManager)
};

class JUCE_API DockLayoutResizerBar : public Component
{
public:
    DockLayoutResizerBar (DockLayoutManager* layoutToUse,
                          int itemIndexInLayout,
                          bool isBarVertical);

    ~DockLayoutResizerBar();

    virtual void hasBeenMoved();

    /** @internal */
    void paint (Graphics&) override;
    /** @internal */
    void mouseDown (const MouseEvent&) override;
    /** @internal */
    void mouseDrag (const MouseEvent&) override;
    /** @internal */
    void mouseUp (const MouseEvent&) override;

private:
    DockLayoutManager* layout;
    int itemIndex, mouseDownPos;
    bool isVertical;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DockLayoutResizerBar)
};

class JUCE_API DockLayout
{
public:
    /** Constructor */
    DockLayout (Component& holder_, bool vertical = false);

    ~DockLayout() noexcept;

    /** Returns the index of the child component */
    inline int indexOf (Component* const child) const { return items.indexOf (child); }

    /** Returns the number of items in the layout */
    inline int getNumItems() const { return items.size(); }

    const Array<Component*>& getItems() const { return items; }

    void insert (int index, Component* const child, int splitType);

    void append (Component* child);

    void move (int sourceIdx, int targetIdx);

    void remove (Component* const child);

    void clear();

    void layoutItems (int x, int y, int w, int h);

    void layoutItems();

    inline void setVertical (bool isNowVertical) { vertical = isNowVertical; }

    inline bool isVertical() const { return vertical; }

    inline int getBarSize() const { return barSize; }
    void setBarSize (int newBarSize);
    String getSizesString() const;
    void setSizes (const String& sizes);

private:
    friend class DockLayoutResizerBar;
    Component& holder;
    bool vertical;
    int barSize = 4;
    DockLayoutManager layout;
    OwnedArray<DockLayoutResizerBar> bars;
    Array<Component*> items;
    Array<Component*> comps;

    void buildComponentArray();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DockLayout)
};

} // namespace element
