/*
    PluginWindow.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#include "controllers/GuiController.h"
#include "engine/GraphNode.h"
#include "gui/GuiCommon.h"
#include "gui/PluginWindow.h"
#include "gui/ContextMenus.h"

namespace Element {
static Array <PluginWindow*> activePluginWindows;

class PluginWindowToolbar : public Toolbar
{
public:
    enum Items {
        BypassPlugin = 1
    };
    
    PluginWindowToolbar() { }
    ~PluginWindowToolbar() { }
};

class PluginWindowContent : public Component,
                            public ComponentListener,
                            public ButtonListener
{
public:
    PluginWindowContent (Component* const _editor, const Node& _node)
        : editor (_editor), object(_node.getGraphNode()), node(_node)
    {
        addAndMakeVisible (toolbar = new PluginWindowToolbar());
        toolbar->setBounds (0, 0, getWidth(), 24);
        
        addAndMakeVisible (editor);
        editor->addComponentListener (this);
        
        addAndMakeVisible (nodeButton);
        nodeButton.setButtonText ("n");
        nodeButton.setColour (TextButton::buttonOnColourId, Colours::red);
        nodeButton.addListener (this);
        
        addAndMakeVisible (bypassButton);
        bypassButton.setButtonText ("b");
        bypassButton.setToggleState (object->getAudioProcessor()->isSuspended(), dontSendNotification);
        bypassButton.setColour (TextButton::buttonOnColourId, Colours::red);
        bypassButton.addListener (this);
        
        updateSize();
    }
    
    ~PluginWindowContent()
    {
        bypassButton.removeListener (this);
        
        if (object && editor)
        {
            if (auto* proc = object->getAudioProcessor())
                if (auto* const e = dynamic_cast<AudioProcessorEditor*> (editor.get()))
                    proc->editorBeingDeleted (e);
        }
        
        editor      = nullptr;
        toolbar     = nullptr;
        leftPanel   = nullptr;
        rightPanel  = nullptr;
    }
    
    void updateSize()
    {
        const int height = jmax (editor->getHeight(), 100) + toolbar->getHeight();
        setSize (editor->getWidth(), height);
        resized();
    }
    
    void resized() override
    {
        editor->removeComponentListener (this);
        auto r (getLocalBounds());
        
        if (toolbar->getThickness())
        {
            auto r2 = r.removeFromTop (toolbar->getThickness());
            toolbar->setBounds (r2);
            
            auto r3 = r2.withSizeKeepingCentre (r2.getWidth(), 16);
            r3.removeFromRight (4);
            
            nodeButton.setBounds (r3.removeFromRight (16));
            r3.removeFromRight (4);
            
            bypassButton.setBounds (r3.removeFromRight (16));
            r3.removeFromRight (4);
        }
        
        editor->setBounds (0, r.getY(), editor->getWidth(), editor->getHeight());
        editor->addComponentListener (this);
    }
    
    void buttonClicked (Button* button) override
    {
        if (button == &bypassButton)
        {
            const bool desiredBypassState = !object->getAudioProcessor()->isSuspended();
            object->getAudioProcessor()->suspendProcessing (desiredBypassState);
            const bool isNowSuspended = object->getAudioProcessor()->isSuspended();
            bypassButton.setToggleState (isNowSuspended, dontSendNotification);
            node.setProperty (Tags::bypass, isNowSuspended);
        }
        else if (button == &nodeButton)
        {
            NodePopupMenu menu (node);
           #if 1
            menu.clear(); // FIXME: need to have access to app controller via the PluginWindow
                          // Can't use static objects to account for multiple running Element AU/VST
                          // instances
           #endif
            menu.addProgramsMenu();
            if (auto* message = menu.showAndCreateMessage())
                ViewHelpers::postMessageFor (this, message);
        }
    }
    
    void componentMovedOrResized (Component& c, bool wasMoved, bool wasResized) override
    {
        if (editor && editor != &c)
            return;
        if (wasResized)
            updateSize();
        ignoreUnused(wasMoved);
    }
    
    Toolbar* getToolbar() const { return toolbar.get(); }
    
private:
    ScopedPointer<PluginWindowToolbar> toolbar;
    SettingButton bypassButton, nodeButton;
    ScopedPointer<Component> editor, leftPanel, rightPanel;
    GraphNodePtr object;
    Node node;
};

PluginWindow::PluginWindow (Component* const ui, const Node& n)
    : DocumentWindow (n.getName(), LookAndFeel::backgroundColor,
                      DocumentWindow::minimiseButton | DocumentWindow::closeButton, false),
      owner (n.getGraphNode()), node(n)
{
    setUsingNativeTitleBar (true);
    setSize (400, 300);
    setContentOwned (new PluginWindowContent (ui, node), true);
    
    if (node.isValid())
    {
        setTopLeftPosition (node.getValueTree().getProperty ("windowX", Random::getSystemRandom().nextInt (500)),
                            node.getValueTree().getProperty ("windowY", Random::getSystemRandom().nextInt (500)));
        node.getValueTree().setProperty ("windowVisible", true, 0);
    }
    
    setVisible (true);
    addToDesktop();
    
    if (auto* ed = dynamic_cast<AudioProcessorEditor*> (ui))
    {
        setResizable (ed->isResizable(), false);
    }
    
    activePluginWindows.add (this);
}
    
PluginWindow::~PluginWindow()
{
    activePluginWindows.removeFirstMatchingValue (this);
    clearContentComponent();
}

ContentComponent* PluginWindow::getElementContentComponent() const
{
    return nullptr; // FIXME
}
    
void PluginWindow::deleteWindow (const int index, const bool windowVisible)
{
    auto* window = activePluginWindows.getUnchecked (index);
    window->node.setProperty ("windowVisible", windowVisible);
    deleteAndZero (window);
}
    
void PluginWindow::closeCurrentlyOpenWindowsFor (GraphProcessor& proc, const bool windowVisible)
{
    for (int i = 0; i < proc.getNumNodes(); ++i)
        if (auto node = proc.getNode (i))
            for (int i = activePluginWindows.size(); --i >= 0;)
                if (activePluginWindows.getUnchecked(i)->owner == node)
                    { deleteWindow (i, windowVisible); break; }
}

void PluginWindow::closeCurrentlyOpenWindowsFor (GraphNode* const node, const bool windowVisible)
{
    for (int i = activePluginWindows.size(); --i >= 0;)
        if (activePluginWindows.getUnchecked(i)->owner == node)
            { deleteWindow (i, windowVisible); break; }
}

void PluginWindow::closeCurrentlyOpenWindowsFor (const uint32 nodeId, const bool windowVisible)
{
    for (int i = activePluginWindows.size(); --i >= 0;)
        if (activePluginWindows.getUnchecked(i)->owner->nodeId == nodeId)
            { deleteWindow (i, windowVisible); break; }
}

void PluginWindow::closeAllCurrentlyOpenWindows (const bool windowVisible)
{
    if (activePluginWindows.size() > 0)
    {
        for (int i = activePluginWindows.size(); --i >= 0;)
            deleteWindow (i, windowVisible);
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
    jassertfalse;
    return nullptr;
}

PluginWindow* PluginWindow::createWindowFor (GraphNode* node, Component* ed)
{
    jassertfalse;
    return nullptr;
}

PluginWindow* PluginWindow::getWindowFor (const Node& node)
{
    return getWindowFor (node.getGraphNode());
}

PluginWindow* PluginWindow::createWindowFor (const Node& node)
{
    GraphNodePtr object = node.getGraphNode();
    AudioProcessor* proc = (object != nullptr) ? object->getAudioProcessor() : nullptr;
    if (! proc)
        return nullptr;
    if (!proc->hasEditor())
        return nullptr;
    
    auto* editor = proc->createEditorIfNeeded();
    return (editor != nullptr) ? createWindowFor (node, editor) : nullptr;
}

PluginWindow* PluginWindow::createWindowFor (const Node& n, Component* e) {
    return new PluginWindow (e, n);
}

PluginWindow* PluginWindow::getOrCreateWindowFor (const Node& node)
{
    if (auto* w = getWindowFor (node))
        return w;
    return createWindowFor (node);
}

void PluginWindow::moved()
{
    node.setProperty ("windowX", getX());
    node.setProperty ("windowY", getY());
}

void PluginWindow::closeButtonPressed()
{
    const int index = activePluginWindows.indexOf (this);
    if (index > 0)
    {
        deleteWindow (index, false);
    }
    else
    {
        node.setProperty ("windowVisible", false);
        delete this;
    }
}

}
