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

PluginWindow::PluginWindow (GuiController& g, Component* const ui, const Node& n)
    : DocumentWindow (n.getName(), LookAndFeel::backgroundColor,
                      DocumentWindow::minimiseButton | DocumentWindow::closeButton, false),
      gui (g), owner (n.getGraphNode()), node (n)
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
}
    
PluginWindow::~PluginWindow()
{
    clearContentComponent();
}

ContentComponent* PluginWindow::getElementContentComponent() const
{
    return gui.getContentComponent();
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

void PluginWindow::updateGraphNode (GraphNode *newNode, Component *newEditor)
{
    jassert(nullptr != newNode && nullptr != newEditor);
    owner = newNode;
    setContentOwned (newEditor, true);
}
    
void PluginWindow::moved()
{
    node.setProperty ("windowX", getX());
    node.setProperty ("windowY", getY());
}

void PluginWindow::closeButtonPressed()
{
    gui.closePluginWindow (this);
}

}
