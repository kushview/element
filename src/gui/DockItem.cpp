/*
    DockItem.cpp - This file is part of Element
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
#include "gui/DockItemTabs.h"
#include "gui/DockPanel.h"

namespace element {

class DockItem::DragOverlay : public Component
{
public:
    DragOverlay()
    {
        setRepaintsOnMouseActivity (true);
        resized();
    }

    ~DragOverlay() = default;

    DockPlacement getPlacement (const MouseEvent& event)
    {
        const auto point = event.getPosition().toFloat();
        return getPlacement (point);
    }

    DockPlacement getPlacement (const juce::Point<float>& point)
    {
        if (left.contains (point))
            return DockPlacement::Left;
        if (right.contains (point))
            return DockPlacement::Right;
        if (top.contains (point))
            return DockPlacement::Top;
        if (bottom.contains (point))
            return DockPlacement::Bottom;
        return DockPlacement::Center;
    }

    void visibilityChanged() override { resized(); }

    void paint (Graphics& g) override
    {
        const auto backgroundColor = Colours::grey;
        const auto highlightColor = Colours::blueviolet;
        const auto outlineColor = Colours::black;

        g.setOpacity (0.40);
        g.fillAll (backgroundColor);

        bool hasPaintedMouseArea = false;
        Path* paths[] = { &left, &right, &top, &bottom };
        for (int i = 0; i < 4; ++i)
        {
            if (paths[i]->contains (mouse))
            {
                g.setColour (highlightColor);
                g.fillPath (*paths[i]);
                hasPaintedMouseArea = true;
                break;
            }
        }

        if (! hasPaintedMouseArea && center.contains (mouse))
        {
            g.setColour (highlightColor);
            g.fillRect (center);
        }

        auto bounds = getLocalBounds().toFloat();
        g.setColour (outlineColor);
        g.drawRect (getLocalBounds(), 1);

        g.drawLine ({ bounds.getTopLeft(), center.getTopLeft() }, 1.f);
        g.drawLine ({ bounds.getTopRight(), center.getTopRight() }, 1.f);
        g.drawLine ({ bounds.getBottomLeft(), center.getBottomLeft() }, 1.f);
        g.drawLine ({ bounds.getBottomRight(), center.getBottomRight() }, 1.f);
        g.drawRect (center, 1);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().toFloat();
        center = getLocalBounds().reduced (spacingX, spacingY).toFloat();
        left.clear();
        right.clear();
        top.clear();
        bottom.clear();

        left.startNewSubPath (bounds.getTopLeft());
        left.lineTo (bounds.getBottomLeft());
        left.lineTo (center.getBottomLeft());
        left.lineTo (center.getTopLeft());
        left.closeSubPath();

        right.startNewSubPath (bounds.getTopRight());
        right.lineTo (bounds.getBottomRight());
        right.lineTo (center.getBottomRight());
        right.lineTo (center.getTopRight());
        right.closeSubPath();

        top.startNewSubPath (bounds.getTopLeft());
        top.lineTo (bounds.getTopRight());
        top.lineTo (center.getTopRight());
        top.lineTo (center.getTopLeft());
        top.closeSubPath();

        bottom.startNewSubPath (bounds.getBottomLeft());
        bottom.lineTo (bounds.getBottomRight());
        bottom.lineTo (center.getBottomRight());
        bottom.lineTo (center.getBottomLeft());
        bottom.closeSubPath();
    }

private:
    friend class DockItem;
    int spacingX = 30;
    int spacingY = 30;
    Rectangle<float> center;
    Path left, right, top, bottom;
    juce::Point<float> mouse;
};

class DockItem::ChildListener : public MouseListener
{
public:
    ChildListener (Dock& d, DockItem& i) : dock (d), item (i) {}
    ~ChildListener() {}
    void mouseDown (const MouseEvent& event) override
    {
        item.setSelected (true);
    }

private:
    Dock& dock;
    DockItem& item;
};

DockItem::DockItem (Dock& parent, DockPanel* panel)
    : dock (parent)
{
    tabs.reset (new DockItemTabs());
    addAndMakeVisible (tabs.get());

    overlay.reset (new DragOverlay());
    addChildComponent (overlay.get(), 9000);
    overlay->setAlpha (0.50);

    if (panel != nullptr && ! panels.contains (panel))
    {
        panels.add (panel);
        refreshPanelContainer();
        tabs->setCurrentTabIndex (panels.indexOf (panel));
    }

    listener.reset (new ChildListener (parent, *this));
    addMouseListener (listener.get(), true);
}

DockItem::~DockItem()
{
    removeMouseListener (listener.get());
    listener.reset();

    overlay = nullptr;
    tabs->clearTabs();
    tabs = nullptr;
    panels.clear();
}

void DockItem::setSelected (bool shouldBeSelected, bool deselectOthers)
{
    ignoreUnused (deselectOthers);
    if (selected == shouldBeSelected)
        return;

    if (shouldBeSelected && deselectOthers)
    {
        for (auto* item : dock.items)
            item->setSelected (false);
    }

    selected = shouldBeSelected;
    repaint();
}

int DockItem::getCurrentPanelIndex() const { return tabs->getCurrentTabIndex(); }
DockPanel* DockItem::getCurrentPanel() const
{
    return dynamic_cast<DockPanel*> (tabs->getCurrentContentComponent());
}

void DockItem::dockTo (DockItem* const target, DockPlacement placement)
{
    if (target->getNumPanels() > 0)
    {
        for (auto* const panel : panels)
            panel->dockTo (target, placement);
    }
    else
    {
        // TODO: unhandled docking condition
        jassertfalse;
    }
}

void DockItem::setCurrentPanelIndex (int panel)
{
    if (getCurrentPanelIndex() == panel)
        return;
    tabs->setCurrentTabIndex (jlimit (0, panels.size() - 1, panel));
}

void DockItem::reset()
{
    tabs->clearTabs();
    panels.clear();
    refreshPanelContainer();
}

void DockItem::detach (DockPanel* const panel)
{
    if (! panels.contains (panel))
        return;

    panels.removeFirstMatchingValue (panel);
    for (int i = tabs->getNumTabs(); --i >= 0;)
        if (panel == dynamic_cast<DockPanel*> (tabs->getTabContentComponent (i)))
        {
            tabs->removeTab (i);
            break;
        }

    if (panels.size() == 0)
        detach();
    else
        refreshPanelContainer();
}

void DockItem::detach()
{
    if (auto* area = getParentArea())
    {
        area->remove (this);
        area->resized();
    }

    jassert (getParentComponent() == nullptr);
}

void DockItem::movePanelsTo (DockItem* const target)
{
    Array<DockPanel*> tempPanels;
    for (auto* const panel : panels)
        tempPanels.add (panel);
    panels.clear();
    refreshPanelContainer();

    for (auto* const panel : tempPanels)
        target->panels.add (panel);
    tempPanels.clearQuick();
    target->refreshPanelContainer();
}

void DockItem::refreshPanelContainer (DockPanel* const panelToSelect)
{
    auto lastIndex = tabs->getCurrentTabIndex();
    tabs->clearTabs();
    const auto colour = findColour (DocumentWindow::backgroundColourId);

    for (auto* const panel : panels)
        tabs->addTab (panel->getName(), colour, panel, false);

    if (panelToSelect != nullptr && panels.contains (panelToSelect))
        lastIndex = panels.indexOf (panelToSelect);

    if (panels.size() > 0)
        tabs->setCurrentTabIndex (jlimit (0, panels.size() - 1, lastIndex));
}

ValueTree DockItem::getState() const
{
    ValueTree state ("item");
    state.setProperty ("bounds", getLocalBounds().toString(), nullptr)
        .setProperty ("mode", "tabs", nullptr)
        .setProperty ("panel", getCurrentPanelIndex(), nullptr);
    for (auto* panel : panels)
        state.addChild (panel->getState(), -1, nullptr);
    return state;
}

void DockItem::paint (Graphics& g)
{
    if (selected)
    {
        g.setColour (findColour (DockItem::selectedHighlightColourId));
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.4), 1.5f, 1.4f);
    }
}

void DockItem::resized()
{
    const int indent = 2;
    if (panels.size() > 0)
    {
        auto ir = getLocalBounds().reduced (indent);
        if (overlay->isVisible())
            overlay->centreWithSize (getWidth() - 2, getHeight() - 2);
        tabs->setBounds (ir);
    }
}

void DockItem::mouseDown (const MouseEvent& ev)
{
    if (DockPanel* const panel = getCurrentPanel())
        dock.startDragging (panel);
}

bool DockItem::isInterestedInDragSource (const SourceDetails& details)
{
    return details.description.toString() == "DockPanel" || details.description.toString() == "DockItem";
}

void DockItem::itemDropped (const SourceDetails& dragSourceDetails)
{
    overlay->setVisible (false);

    auto* const panel = dynamic_cast<DockPanel*> (dragSourceDetails.sourceComponent.get());
    auto* const item = (panel != nullptr) ? panel->findParentComponentOfClass<DockItem>() : nullptr;
    if (panel == nullptr || item == nullptr)
        return;

    DockPlacement placement = overlay->getPlacement (dragSourceDetails.localPosition.toFloat());

    const bool isMyPanel = panels.contains (panel);

    // same item center, don't add to container of self
    if (isMyPanel && placement == DockPlacement::Center)
        return;

    panel->dockTo (this, placement);

    refreshPanelContainer();
}

void DockItem::itemDragEnter (const SourceDetails&)
{
    overlay->toFront (true);
    overlay->setVisible (true);
    resized();
}

void DockItem::itemDragMove (const SourceDetails& dragSourceDetails)
{
    overlay->mouse = dragSourceDetails.localPosition.toFloat();
    overlay->repaint();
}

void DockItem::itemDragExit (const SourceDetails&)
{
    overlay->setVisible (false);
    resized();
}

bool DockItem::shouldDrawDragImageWhenOver() { return false; }

} // namespace element
