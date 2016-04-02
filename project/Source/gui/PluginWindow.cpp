/*
    PluginWindow.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.

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

#include "gui/PluginWindow.h"

namespace Element {
static Array <PluginWindow*> activePluginWindows;

class PluginWindowToolbar : public Toolbar
{
public:
    PluginWindowToolbar() { }
    ~PluginWindowToolbar() { }
    
};

class PluginWindowContent : public Component
{
public:
    PluginWindowContent (Component* const _editor)
        : editor (_editor)
    {
        addAndMakeVisible (toolbar = new PluginWindowToolbar());
        addAndMakeVisible (editor);
        setSize (editor->getWidth(), editor->getHeight());
        toolbar->setBounds (0, 0, getWidth(), 48);
        resized();
    }
    
    ~PluginWindowContent() { }
    
    void resized()
    {
        Rectangle<int> r (getLocalBounds());
        
        if (toolbar->getThickness())
        {
            toolbar->setBounds (r.removeFromTop (toolbar->getThickness()));
        }
        
        editor->setBounds (r);
    }
    
    Toolbar* getToolbar() const { return toolbar.get(); }
    
private:
    ScopedPointer<PluginWindowToolbar> toolbar;
    ScopedPointer<Component> editor, leftPanel, rightPanel;
};

PluginWindow::PluginWindow (Component* const ui, GraphNode* node)
    : DocumentWindow (ui->getName(), Colours::lightgrey,
                      DocumentWindow::minimiseButton | DocumentWindow::closeButton, true),
      owner (node)
{
    setSize (400, 300);
    setContentOwned (ui, true);
    setTopLeftPosition (owner->properties.getWithDefault ("windowLastX", Random::getSystemRandom().nextInt (500)),
                        owner->properties.getWithDefault ("windowLastY", Random::getSystemRandom().nextInt (500)));
    owner->properties.set ("windowVisible", true);
    setVisible (true);
    activePluginWindows.add (this);
}

PluginWindow::~PluginWindow()
{
    clearContentComponent();
    activePluginWindows.removeFirstMatchingValue (this);
}

void PluginWindow::closeCurrentlyOpenWindowsFor (GraphNode* const node)
{
    if (node)
        closeCurrentlyOpenWindowsFor (node->nodeId);
}

void PluginWindow::closeCurrentlyOpenWindowsFor (const uint32 nodeId)
{
    for (int i = activePluginWindows.size(); --i >= 0;)
        if (activePluginWindows.getUnchecked(i)->owner->nodeId == nodeId)
            { delete activePluginWindows.getUnchecked(i); break; }
}

void PluginWindow::closeAllCurrentlyOpenWindows()
{
    if (activePluginWindows.size() > 0)
    {
        for (int i = activePluginWindows.size(); --i >= 0;)
            delete activePluginWindows.getUnchecked(i);

        Component dummyModalComp;
        dummyModalComp.enterModalState();
        MessageManager::getInstance()->runDispatchLoopUntil (50);
    }
}

PluginWindow* PluginWindow::getOrCreateWindowFor (GraphNode* node)
{
    if (PluginWindow* win = getWindowFor (node))
        return win;
    return createWindowFor (node);
}

Toolbar* PluginWindow::getToolbar() const
{
    if (PluginWindowContent* pwc = dynamic_cast<PluginWindowContent*> (getContentComponent()))
        return pwc->getToolbar();
    return nullptr;
}

void PluginWindow::resized()
{
    DocumentWindow::resized();
}

PluginWindow* PluginWindow::getWindowFor (GraphNode* node)
{
    for (int i = activePluginWindows.size(); --i >= 0;)
        if (activePluginWindows.getUnchecked(i)->owner == node)
            return activePluginWindows.getUnchecked(i);

    return nullptr;
}

PluginWindow* PluginWindow::getFirstWindow()
{
    return activePluginWindows.getFirst();
}
    
void PluginWindow::updateGraphNode (GraphNode *newNode, Component *newEditor)
{
    jassert(nullptr != newNode && nullptr != newEditor);
    owner = newNode;
    setContentOwned (newEditor, true);
}
    
PluginWindow* PluginWindow::createWindowFor (GraphNode* node)
{
    AudioPluginInstance* plug (node->getAudioPluginInstance());
    if (! plug->hasEditor())
        return nullptr;
    
    AudioProcessorEditor* editor = plug->createEditorIfNeeded();
    return (editor != nullptr) ? new PluginWindow (editor, node) : nullptr;
}

PluginWindow* PluginWindow::createWindowFor (GraphNode* node, Component* ed)
{
    return new PluginWindow (ed, node);
}

void PluginWindow::moved()
{
    owner->properties.set ("windowLastX", getX());
    owner->properties.set ("windowLastY", getY());
}

void PluginWindow::closeButtonPressed()
{
    if (owner) {
        owner->properties.set ("windowVisible", false);
    }
    delete this;
}

}
