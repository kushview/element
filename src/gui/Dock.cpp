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
#include "gui/DockPanel.h"
#include "gui/DockContainer.h"
#include "gui/DockWindow.h"
#include "gui/DockItemTabs.h"

namespace element {

#define KV_DEBUG_DOCK_ORPHANS 0

struct SortDockInfoByName
{
    static int compareElements (DockPanelInfo* first, DockPanelInfo* second)
    {
        return first->name < second->name ? -1 : first->name == second->name ? 0
                                                                             : 1;
    }
};

Dock::Dock()
{
    container.reset (new DockContainer (*this));
    addAndMakeVisible (container.get());
}

Dock::~Dock()
{
    container.reset (nullptr);
}

DockItem* Dock::getSelectedItem() const
{
    for (auto* const item : items)
        if (item->selected)
            return item;
    return nullptr;
}

void Dock::registerPanelType (DockPanelType* newType)
{
    jassert (newType != nullptr);
    jassert (! types.contains (newType));
    types.add (newType);
    newType->getAllTypes (available);
    SortDockInfoByName compare;
    available.sort (compare, false);
}

DockPanelInfo Dock::getPanelInfo (const String& panelID) const noexcept
{
    for (const auto* info : available)
        if (info->identifier.toString() == panelID)
            return *info;
    return {};
}

DockArea* Dock::createArea (const bool isVertical)
{
    if (auto* area = areas.add (new DockArea()))
    {
        area->setVertical (isVertical);
        return area;
    }

    return nullptr;
}

DockArea* Dock::getOrCreateArea (const bool isVertical, DockArea* areaToSkip)
{
    DockArea* retArea = nullptr;
    for (auto* const area : areas)
        if (area->getNumItems() <= 0 && area->getParentArea() == nullptr)
        {
            retArea = area;
            break;
        }

    if (retArea == nullptr || retArea == container->getRootArea() || (areaToSkip != nullptr && retArea == areaToSkip))
    {
        retArea = createArea (isVertical);
    }

    if (retArea)
    {
        retArea->setVertical (isVertical);
    }

    return retArea;
}

DockItem* Dock::getOrCreateItem (DockPanel* const panel)
{
    DockItem* dockItem = nullptr;
    for (auto* const item : items)
        if (item->getNumPanels() <= 0 && item->getParentComponent() == nullptr)
        {
            dockItem = item;
            break;
        }
    if (! dockItem)
        dockItem = items.add (new DockItem (*this));

    if (dockItem != nullptr)
    {
        dockItem->reset();
        if (panel != nullptr)
        {
            dockItem->panels.add (panel);
            dockItem->refreshPanelContainer();
        }
    }

    return dockItem;
}

DockPanel* Dock::getOrCreatePanel (const String& panelType)
{
    DockPanelInfo* panelInfo = nullptr;
    for (auto* const info : available)
    {
        if (info->identifier == Identifier (panelType))
            panelInfo = info;
        if (panelInfo != nullptr)
            break;
    }

    if (! panelInfo)
        return nullptr;

    DockPanel* panel = nullptr;
    for (auto* type : types)
    {
        panel = type->createPanel (*panelInfo);
        if (panel != nullptr)
            break;
    }

    if (panel != nullptr)
    {
        if (panel->getName().isEmpty())
            panel->setName (panelInfo->name);
        if (panel->getName().isEmpty())
        {
            jassertfalse;
        } // you might want to give your panel and panel type a name

        panel->identifier = panelType;

        panels.add (panel);
        if (onPanelAdded)
            onPanelAdded (panel);
    }

    return panel;
}

void Dock::resized()
{
    container->setBounds (getLocalBounds());
}

bool Dock::keyPressed (const KeyPress& press)
{
    if (press.getKeyCode() == KeyPress::escapeKey)
    {
        // noop. JUCE doesn't have a way to cancel drag operations yet
    }

    return false;
}

void Dock::dragOperationStarted (const DragAndDropTarget::SourceDetails& details)
{
    //    if (auto* panel = dynamic_cast<DockPanel*> (details.sourceComponent.get()))
    //        if (auto* item = dynamic_cast<DockItem*> (panel->findParentComponentOfClass<DockItem>()))
    //            item->setMouseCursor (MouseCursor::CopyingCursor);
}

void Dock::dragOperationEnded (const DragAndDropTarget::SourceDetails& details)
{
    //    if (auto* panel = dynamic_cast<DockPanel*> (details.sourceComponent.get()))
    //        if (auto* item = dynamic_cast<DockItem*> (panel->findParentComponentOfClass<DockItem>()))
    //            item->setMouseCursor (MouseCursor::NormalCursor);
}

DockItem* Dock::createItem (const Identifier& panelID)
{
    if (auto* panel = getOrCreatePanel (panelID.toString()))
        return getOrCreateItem (panel);
    return nullptr;
}

DockItem* Dock::createItem (const Identifier& panelID, DockPlacement placement)
{
    return createItem (panelID.toString(), placement);
}

DockItem* Dock::createItem (const String& panelType, DockPlacement placement)
{
    auto* panel = getOrCreatePanel (panelType);

    if (! panel)
    {
        jassertfalse;
        return nullptr;
    }

    if (placement.isFloating())
    {
        auto* window = windows.add (new DockWindow (*this));
        auto* item = getOrCreateItem (panel);
        window->setBackgroundColour (findColour (DocumentWindow::backgroundColourId).darker());
        window->centreWithSize (window->getWidth(), window->getHeight());
        window->dockItem (item, DockPlacement::Top);
        window->setVisible (true);
        window->addToDesktop();
        window->toFront (true);
        return item;
    }

    if (! placement.isDirectional())
        return nullptr;

    auto* item = getOrCreateItem (panel);
    if (item && ! container->dockItem (item, placement))
    {
        item->reset();
        item = nullptr;
    }

    resized();
    return item;
}

void Dock::selectPanel (DockPanel* panel)
{
    if (panel == nullptr)
        return;

    for (auto* item : items)
    {
        for (int j = 0; j < item->getNumPanels(); ++j)
        {
            if (panel == item->getPanel (j))
            {
                item->setSelected (true, true);
                item->setCurrentPanelIndex (j);
                return;
            }
        }
    }
}

void Dock::showPanel (DockPanel* panel)
{
    if (panel == nullptr)
        return;

    for (auto* item : items)
    {
        for (int j = 0; j < item->getNumPanels(); ++j)
        {
            if (panel == item->getPanel (j))
            {
                item->setCurrentPanelIndex (j);
                return;
            }
        }
    }
}

void Dock::undockPanel (DockPanel* panel)
{
    auto screenBounds = panel->getScreenBounds();
    if (auto parentItem = panel->findParentComponentOfClass<DockItem>())
        parentItem->detach (panel);

    auto* window = windows.add (new DockWindow (*this));
    auto* item = items.add (new DockItem (*this, panel));
    window->setBackgroundColour (findColour (DocumentWindow::backgroundColourId).darker());
    window->dockItem (item, DockPlacement::Top);
    window->setContentComponentSize (screenBounds.getWidth(), screenBounds.getHeight());
    window->setTopLeftPosition (jmax (0, screenBounds.getX() - window->getTitleBarHeight()),
                                screenBounds.getY());
    window->setVisible (true);
    window->addToDesktop();
    window->toFront (true);
}

void Dock::removePanel (DockPanel* panel)
{
    if (panel == nullptr)
        return;

    using DeleterType = std::unique_ptr<DockPanel, std::function<void (DockPanel*)>>;
    DeleterType deleter (panel, [this, panel] (DockPanel* ptr) {
        panels.removeObject (panel);
    });

    auto* const item = panel->findParentComponentOfClass<DockItem>();

    if (item == nullptr)
        return;

    item->detach (panel);
    jassert (panel->getParentComponent() == nullptr);

    if (auto* const area = item->getParentArea())
    {
        auto* const parentArea = item->getParentArea();
        if (parentArea && area->getNumItems() <= 0)
            parentArea->remove (area);
    }
}

void Dock::removeOrphanObjects()
{
#if KV_DEBUG_DOCK_ORPHANS
    const int sizeBefore = panels.size() + items.size() + areas.size();
    DBG ("[KV] dock: purging orphans...");
    DBG ("[KV] dock: areas  : " << areas.size());
    DBG ("[KV] dock: items  : " << items.size());
    DBG ("[KV] dock: panels : " << panels.size());
#endif

    for (int i = areas.size(); --i >= 0;)
    {
        auto* const area = areas.getUnchecked (i);
        if (area->getNumItems() <= 0)
            if (auto* const parent = area->getParentArea())
                parent->remove (area);
    }

    OwnedArray<Component> deleter;
    Array<DockPanel*> panelsToDelete;
    for (int i = areas.size(); --i >= 0;)
    {
        auto* const area = areas.getUnchecked (i);
        if (area == container->getRootArea())
            continue;

        if (container->contains (area))
            continue;

        bool isInDockWindow = false;
        for (auto* const dw : windows)
            if (dw->contains (area))
            {
                isInDockWindow = true;
                break;
            }
        if (isInDockWindow)
            continue;

        for (int j = 0; j < area->getNumItems(); ++j)
        {
            if (auto* const item = dynamic_cast<DockItem*> (area->getItem (j)))
            {
                for (int k = 0; k < item->getNumPanels(); ++k)
                {
                    if (auto* const panel = item->getPanel (k))
                    {
                        panelsToDelete.add (panel);
#if KV_DEBUG_DOCK_ORPHANS
                        DBG ("[KV] dock: orphan panel: " << panel->getName());
#endif
                    }
                }

                area->remove (item);
                item->panels.clear();
                item->tabs->clearTabs();
                jassert (items.contains (item) && item->panels.size() <= 0 && item->tabs->getNumTabs() <= 0);
                items.removeObject (item, true);
            }
        }

        deleter.add (areas.removeAndReturn (i));
    }

    deleter.clear();
    for (auto* panel : panelsToDelete)
        panels.removeObject (panel, true);
    panelsToDelete.clear();

    for (int i = items.size(); --i >= 0;)
    {
        auto* const item = items.getUnchecked (i);
        if (! item->isShowing())
        {
            for (int j = 0; j < item->getNumPanels(); ++j)
            {
                auto* const panel = item->getPanel (j);
#if KV_DEBUG_DOCK_ORPHANS
                DBG ("[KV] dock: hidden panel: " << panel->getName());
#endif
                panelsToDelete.add (panel);
            }
            item->panels.clear();
            item->tabs->clearTabs();
            jassert (items.contains (item) && item->panels.size() <= 0 && item->tabs->getNumTabs() <= 0);
            items.remove (i);
        }
    }

    for (auto* panel : panelsToDelete)
        panels.removeObject (panel, true);
    panelsToDelete.clear();

#if KV_DEBUG_DOCK_ORPHANS
    const int sizeAfter = panels.size() + items.size() + areas.size();
    DBG ("[KV] dock: purged " << (sizeBefore - sizeAfter) << " orphans.");
    DBG ("[KV] dock: areas  : " << areas.size());
    DBG ("[KV] dock: items  : " << items.size());
    DBG ("[KV] dock: panels : " << panels.size());
#endif
}

void Dock::startDragging (DockPanel* const panel)
{
    jassert (panel != nullptr);
    Image image (Image::ARGB, 1, 1, true);
    DragAndDropContainer::startDragging ("DockPanel", panel, juce::ScaledImage (image), true);
}

ValueTree Dock::getState() const
{
    ValueTree state ("dock");
    state.setProperty ("bounds", getLocalBounds().toString(), nullptr);
    state.addChild (container->root->getState(), -1, nullptr);
    for (auto* const window : windows)
    {
        if (window->container.get() != nullptr && window->container->root.getComponent() != nullptr)
        {
            ValueTree winState ("window");
            winState.setProperty ("position", window->getWindowStateAsString(), nullptr);
            winState.appendChild (window->container->root->getState(), nullptr);
            state.appendChild (winState, nullptr);
        }
    }
    return state;
}

void Dock::loadArea (DockArea& area, const ValueTree& state)
{
    const auto sizes = state.getProperty ("sizes").toString();
    const auto barSize = (int) state.getProperty ("barSize", 4);
    area.setBounds (Dock::getBounds (state));
    area.layout.clear();
    area.layout.setBarSize (barSize);

    if (sizes.isNotEmpty())
        area.layout.setSizes (sizes);

    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        const auto child = state.getChild (i);
        if (child.hasType ("item"))
        {
            auto* item = getOrCreateItem();
            loadItem (*item, child);
            area.append (item);
        }
        else if (child.hasType ("area"))
        {
            jassert (area.isVertical() != (bool) child["vertical"]);
            auto* newArea = createArea (! area.isVertical());
            jassert (newArea != &area);
            loadArea (*newArea, child);
            area.append (newArea);
        }
    }

    if (sizes.isNotEmpty())
        area.layout.setSizes (sizes);

    area.resized();
}

void Dock::loadItem (DockItem& item, const ValueTree& state)
{
    item.reset();
    item.setBounds (Dock::getBounds (state));
    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        const auto child = state.getChild (i);

        if (child.hasType ("panel"))
        {
            if (auto* panel = getOrCreatePanel (child["type"].toString()))
            {
                loadPanel (*panel, child);
                item.panels.addIfNotAlreadyThere (panel);
            }
            else
            {
                // couldn't create panel
                jassertfalse;
            }
        }
        else
        {
            DBG (child.toXmlString());
            jassertfalse; // not a panel?
        }
    }

    item.refreshPanelContainer();
    item.setCurrentPanelIndex ((int) state.getProperty ("panel", 0));
    item.resized();
}

void Dock::loadPanel (DockPanel& panel, const ValueTree& state)
{
    panel.setName (state.getProperty ("name", "Panel").toString());
    panel.setBounds (Dock::getBounds (state));
}

bool Dock::applyState (const ValueTree& state)
{
    if (! state.hasType ("dock"))
        return false;

    std::unique_ptr<DockContainer> newContainer;
    OwnedArray<DockWindow> newWindows;

    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        const auto child = state.getChild (i);
        if (child.hasType ("area"))
        {
            if (newContainer == nullptr)
            {
                newContainer.reset (new DockContainer (*this));
                newContainer->setBounds (Dock::getBounds (state));
                newContainer->resized();
                auto* area (newContainer->getRootArea());
                jassert (area);
                area->setVertical ((bool) child.getProperty ("vertical", true));
                area->setBounds (Dock::getBounds (child));
                loadArea (*area, child);
            }
            else
            {
                // root state should only have one area child
                jassertfalse;
            }
        }
        else if (child.hasType ("window"))
        {
            DBG ("load dock window");
            DBG (child.toXmlString());
            auto* const window = newWindows.add (new DockWindow (*this));
            ValueTree data = child.getChildWithName ("area");
            jassert (window->container && window->container->getRootArea() != nullptr);
            window->restoreWindowStateFromString (child.getProperty ("position").toString());
            loadArea (*window->container->getRootArea(), data);
        }
    }

    if (! newContainer)
        return false;

    removeChildComponent (container.get());
    container.swap (newContainer);
    addAndMakeVisible (container.get());
    resized();

    windows.swapWith (newWindows);
    newWindows.clear();
    for (auto* window : windows)
    {
        window->setVisible (true);
        window->addToDesktop();
        window->resized();
        window->repaint();
    }

    triggerAsyncUpdate();
    return true;
}

} // namespace element
