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
#include "gui/DockLayout.h"
#include "gui/DockPlacement.h"

namespace element {

class DockArea;
class DockContainer;
class DockItem;
class DockItemTabs;
class DockLayout;
class DockPanel;
class DockPanelType;
class DockWindow;

struct DockPanelInfo;

class JUCE_API Dock : public Component,
                      public DragAndDropContainer,
                      private AsyncUpdater
{
public:
    enum SplitType
    {
        NoSplit,
        SplitBefore,
        SplitAfter
    };

    Dock();
    virtual ~Dock();

    std::function<void (DockPanel*)> onPanelAdded;

    inline static SplitType getSplitType (const DockPlacement placement)
    {
        SplitType split = NoSplit;

        switch (placement)
        {
            case DockPlacement::Top:
            case DockPlacement::Left:
                split = SplitAfter;
                break;
            case DockPlacement::Bottom:
            case DockPlacement::Right:
                split = SplitBefore;
                break;
            default:
                break;
        }

        return split;
    }

    inline static String getSplitString (const int splitType)
    {
        switch (splitType)
        {
            case NoSplit:
                return "No split";
                break;
            case SplitBefore:
                return "Split before";
                break;
            case SplitAfter:
                return "Split after";
                break;
        }

        return "Unknown Split";
    }

    inline static String getDirectionString (const int placement)
    {
        const DockPlacement info (placement);
        return info.toString();
    }

    inline static Rectangle<int> getBounds (const ValueTree& data)
    {
        return Rectangle<int>::fromString (data.getProperty ("bounds").toString());
    }

    void registerPanelType (DockPanelType* newType);

    const OwnedArray<DockPanelType>& getPanelTypes() const { return types; }
    const OwnedArray<DockPanelInfo>& getPanelDescriptions() const { return available; }
    DockPanelInfo getPanelInfo (const String& panelID) const noexcept;

    /** Create an item by panel type and dock it to the given placement */
    DockItem* createItem (const Identifier& panelID);
    DockItem* createItem (const String& panelID, DockPlacement placement);
    DockItem* createItem (const Identifier& panelID, DockPlacement placement);

    DockItem* getSelectedItem() const;

    /** Start a drag operation on the passed in DockPanel */
    void startDragging (DockPanel* const panel);

    ValueTree getState() const;
    bool applyState (const ValueTree& state);

    /** Returns the total number of active panels */
    int getNumPanels() const { return panels.size(); }

    /** Get a panel by index */
    DockPanel* getPanel (const int index) const { return panels[index]; }

    /** Select a panel by object */
    void selectPanel (DockPanel* panel);

    /** Show a panel, but not select the item */
    void showPanel (DockPanel* panel);

    /** @internal */
    inline virtual void paint (Graphics& g) override
    {
        g.fillAll (findColour (DocumentWindow::backgroundColourId).darker());
    }

    enum ColourIds
    {
        backgroundColourId = 0x90005000
    };

    /** @internal */
    void resized() override;
    /** @internal */
    bool keyPressed (const KeyPress&) override;
    /** @internal */
    void mouseMove (const MouseEvent& ev) override {}
    /** @internal */
    void dragOperationStarted (const DragAndDropTarget::SourceDetails& details) override;
    /** @internal */
    void dragOperationEnded (const DragAndDropTarget::SourceDetails& details) override;

private:
    friend class DockItem;
    friend class DockContainer;
    friend class DockPanel;
    friend class DockWindow;

    OwnedArray<DockPanelType> types;
    OwnedArray<DockPanelInfo> available;
    std::unique_ptr<DockContainer> container;
    OwnedArray<DockWindow> windows;
    OwnedArray<DockArea> areas;
    OwnedArray<DockItem> items;
    OwnedArray<DockPanel> panels;

    DockArea* createArea (const bool isVertical);
    DockArea* getOrCreateArea (const bool isVertical = true, DockArea* areaToSkip = nullptr);
    DockItem* getOrCreateItem (DockPanel* const panel = nullptr);
    DockPanel* getOrCreatePanel (const String&);

    void undockPanel (DockPanel*);
    void removePanel (DockPanel*);

    void loadArea (DockArea&, const ValueTree&);
    void loadItem (DockItem&, const ValueTree&);
    void loadPanel (DockPanel&, const ValueTree&);

    void removeOrphanObjects();
    void handleAsyncUpdate() override { removeOrphanObjects(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Dock)
};

class JUCE_API DockArea : public Component
{
public:
    /** Destructor */
    ~DockArea();

    /** Returns the index of the given item */
    inline int indexOf (DockItem* const item) const { return layout.indexOf ((Component*) item); }

    /** Returns the index of the given area */
    inline int indexOf (DockArea* const area) const { return layout.indexOf ((Component*) area); }

    /** Returns the number of items in the layout */
    inline int getNumItems() const { return layout.getNumItems(); }

    void moveItem (int source, int target);

    /** Get an Item */
    Component* getItem (const int index) const;

    /** Returns the parent area containing this DockArea */
    DockArea* getParentArea() const { return findParentComponentOfClass<DockArea>(); }

    /** Gets the sizes as a string */
    String getSizesString() const { return layout.getSizesString(); }

    /** Sets the sizes from a string */
    void setSizes (const String& sizes) { layout.setSizes (sizes); }

    /** Append a DockArea to the end of the layout */
    void append (DockArea* const area);

    /** Append a DockItem to the end of the layout */
    void append (DockItem* const item);

    /** Insert a DockItem at a specific location */
    void insert (int index, Component* const item, Dock::SplitType split = Dock::NoSplit);

    /** Insert a DockItem at a specific location */
    void insert (int index, DockItem* const item, Dock::SplitType split = Dock::NoSplit);

    /** Insert a DockArea at a specific location */
    void insert (int index, DockArea* const area, Dock::SplitType split = Dock::NoSplit);

    /** Remove an item from the area */
    void remove (Component* const item);

    /** Remove a dock item from the area */
    void remove (DockItem* const item);

    /** Remove a child area from this area */
    void remove (DockArea* const area);

    /** Returns true if this area has a top to bottom layout */
    bool isVertical() const { return layout.isVertical(); }

    /** @internal */
    inline virtual void paint (Graphics&) override {}
    /** @internal */
    void resized() override;

private:
    friend class Dock;
    friend class DockContainer;
    friend class DockItem;
    friend class DockPanel;

    explicit DockArea (const bool vertical = false);
    DockArea (DockPlacement placement);
    ValueTree getState() const;
    void setVertical (const bool vertical);

    DockLayout layout;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DockArea)
};

class JUCE_API DockItem : public Component,
                          public DragAndDropTarget
{
public:
    enum DisplayMode
    {
        Tabs = 0
    };

    /** Destructor */
    virtual ~DockItem();

    /** Dock all panels in this item to the target item */
    void dockTo (DockItem* target, DockPlacement placement);

    /** Returns the dock */
    Dock* getDock() const { return const_cast<Dock*> (&dock); }

    /** Returns the DockArea which contains this item */
    DockArea* getParentArea() const { return dynamic_cast<DockArea*> (getParentComponent()); }

    /** Selects this Item */
    void setSelected (bool shouldBeSelected, bool deselectOthers = true);

    /** Returns true if this item is selected */
    bool isSelected() const { return selected; }

    /** Returns the number of panels in this item's container */
    int getNumPanels() const { return panels.size(); }

    /** Returns one of the dock panels or nullptr if out of range */
    DockPanel* getPanel (int index) const { return panels[index]; }

    /** Returns the index of a panel, or -1 if not found */
    int indexOf (DockPanel* panel) const noexcept { return panels.indexOf (panel); }

    /** Returns the current panel index */
    int getCurrentPanelIndex() const;

    /** Returns the current panel object */
    DockPanel* getCurrentPanel() const;

    /** Set the current panel index */
    void setCurrentPanelIndex (int panel);

    enum ColourIds
    {
        backgroundColourId = 0xE0005010,
        selectedHighlightColourId = 0xE0005011
    };

    /** @internal */
    void paint (Graphics&) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void mouseDown (const MouseEvent&) override;
    /** @internal */
    bool isInterestedInDragSource (const SourceDetails&) override;
    /** @internal */
    void itemDropped (const SourceDetails&) override;
    /** @internal */
    void itemDragEnter (const SourceDetails&) override;
    /** @internal */
    void itemDragMove (const SourceDetails&) override;
    /** @internal */
    void itemDragExit (const SourceDetails&) override;
    /** @internal */
    bool shouldDrawDragImageWhenOver() override;

private:
    friend class Dock;
    friend class DockArea;
    friend class DockPanel;

    explicit DockItem (Dock& parent, DockPanel* const panel = nullptr);

    Dock& dock;
    DisplayMode displayMode = Tabs;
    bool selected { false };
    bool dragging { false };

    std::unique_ptr<DockItemTabs> tabs;
    Array<DockPanel*> panels;

    ValueTree getState() const;
    void movePanelsTo (DockItem* const target);
    void detach (DockPanel* const panel);
    void detach();
    void refreshPanelContainer (DockPanel* const panelToSelect = nullptr);
    void reset();

    class DragOverlay;
    std::unique_ptr<DragOverlay> overlay;
    class ChildListener;
    std::unique_ptr<ChildListener> listener;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DockItem)
};

} // namespace element
