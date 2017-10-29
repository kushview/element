/*
    PluginWindow.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#include "engine/GraphNode.h"
#include "gui/GuiCommon.h"
#include "gui/PluginWindow.h"

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
    PluginWindowContent (Component* const _editor, GraphNode* _node)
        : editor (_editor), object(_node)
    {
        addAndMakeVisible (toolbar = new PluginWindowToolbar());
        toolbar->setBounds (0, 0, getWidth(), 24);
        
        addAndMakeVisible (editor);
        
        addAndMakeVisible (bypassButton);
        
        bypassButton.setButtonText ("Bypass");
        bypassButton.setToggleState (node.isBypassed(), dontSendNotification);
        bypassButton.setColour (TextButton::buttonOnColourId, Colours::red);
        bypassButton.addListener (this);
        
        setSize (editor->getWidth(), editor->getHeight() + toolbar->getHeight());
        resized();
    }
    
    PluginWindowContent (Component* const _editor, const Node& _node)
        : editor (_editor), object(_node.getGraphNode()), node(_node)
    {
        addAndMakeVisible (toolbar = new PluginWindowToolbar());
        toolbar->setBounds (0, 0, getWidth(), 24);
        
        addAndMakeVisible (editor);
        
        addAndMakeVisible (bypassButton);
        bypassButton.setButtonText ("B");
        bypassButton.setToggleState (object->getAudioProcessor()->isSuspended(), dontSendNotification);
        bypassButton.setColour (TextButton::buttonOnColourId, Colours::red);
        bypassButton.addListener (this);
        
        const int height = jmax (editor->getHeight(), 100) + toolbar->getHeight();
        setSize (editor->getWidth(), height);
        resized();
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
    
    void resized() override
    {
        auto r (getLocalBounds());
        
        if (toolbar->getThickness())
        {
            auto r2 = r.removeFromTop (toolbar->getThickness());
            toolbar->setBounds (r2);
            r2.removeFromRight(4);
            bypassButton.changeWidthToFitText();
            bypassButton.setBounds (r2.removeFromRight(bypassButton.getWidth()).reduced (1));
        }
        
        editor->setBounds (0, r.getY(), getWidth(), editor->getHeight());
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
    }
    
    void componentMovedOrResized (Component&, bool wasMoved, bool wasResized) override { }
    
    Toolbar* getToolbar() const { return toolbar.get(); }
    
private:
    ScopedPointer<PluginWindowToolbar> toolbar;
    TextButton bypassButton;
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
    activePluginWindows.add (this);
}
    
PluginWindow::~PluginWindow()
{
    activePluginWindows.removeFirstMatchingValue (this);
    clearContentComponent();
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
    node.setProperty ("windowVisible", false);
    delete this;
}

}
