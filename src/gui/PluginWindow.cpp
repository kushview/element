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
                            public Button::Listener
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
        
        addAndMakeVisible (powerButton);
        powerButton.setColour (SettingButton::backgroundOnColourId,
                               findColour (SettingButton::backgroundColourId));
        powerButton.setColour (SettingButton::backgroundColourId, Colors::toggleBlue);
        powerButton.getToggleStateValue().referTo (node.getPropertyAsValue (Tags::bypass));
        powerButton.setClickingTogglesState (true);
        powerButton.addListener (this);

        addAndMakeVisible (onTopButton);
        onTopButton.setButtonText ("^");
        onTopButton.setTooltip ("Keep plugin window on top of others");
        onTopButton.addListener (this);

        updateSize();
    }
    
    ~PluginWindowContent() noexcept
    {
        powerButton.removeListener (this);
        
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
        auto r (getLocalBounds().reduced (2));
        
        if (toolbar->getThickness())
        {
            auto r2 = r.removeFromTop (toolbar->getThickness());
            toolbar->setBounds (r2);
            
            auto r3 = r2.withSizeKeepingCentre (r2.getWidth(), 16);
            r3.removeFromRight (4);
            
            nodeButton.setBounds (r3.removeFromRight (16));
            r3.removeFromRight (4);
            
            powerButton.setBounds (r3.removeFromLeft (16));

            r3.removeFromLeft (4);
            onTopButton.setBounds (r3.removeFromLeft (16));

            r3.removeFromRight (4);
        }
        
        editor->setBounds (0, r.getY(), editor->getWidth(), editor->getHeight());
        editor->addComponentListener (this);
    }

    void buttonClicked (Button* button) override
    {
        auto* const proc = getProcessor();
        if (button == &powerButton)
        {
            if (proc && proc->isSuspended() != node.isBypassed())
                proc->suspendProcessing (node.isBypassed());
        }
        else if (button == &nodeButton)
        {
            NodePopupMenu menu (node);
            menu.addSeparator();
            menu.addProgramsMenu();
            menu.addPresetsMenu();
            if (auto* message = menu.showAndCreateMessage())
                ViewHelpers::postMessageFor (this, message);
        }
        else if (button == &onTopButton)
        {
            if (auto* pw = findParentComponentOfClass<PluginWindow>())
            {
                pw->setAlwaysOnTop (! pw->isAlwaysOnTop());
                node.setProperty (Tags::windowOnTop, pw->isAlwaysOnTop());
            }
        }

        stabilizeComponents();
    }
    
    void componentMovedOrResized (Component& c, bool wasMoved, bool wasResized) override
    {
        if (editor && editor != &c)
            return;
        if (wasResized)
            updateSize();
        ignoreUnused (wasMoved);
    }
    
    Toolbar* getToolbar() const { return toolbar.get(); }

    void stabilizeComponents()
    {
        if (auto* pw = findParentComponentOfClass<PluginWindow>())
        {
            onTopButton.setToggleState (pw->isAlwaysOnTop(), dontSendNotification);
        }
    }
private:
    ScopedPointer<PluginWindowToolbar> toolbar;
    SettingButton nodeButton;
    PowerButton powerButton;
    SettingButton onTopButton;
    Value bypassValue;

    ScopedPointer<Component> editor, leftPanel, rightPanel;
    GraphNodePtr object;
    Node node;

    AudioProcessor* getProcessor() { return (object != nullptr) ? object->getAudioProcessor() : nullptr; }
};

PluginWindow::PluginWindow (GuiController& g, Component* const ui, const Node& n)
    : DocumentWindow (n.getName(), LookAndFeel::backgroundColor,
                      DocumentWindow::minimiseButton | DocumentWindow::closeButton, true),
      gui (g), owner (n.getGraphNode()), node (n)
{
    setLookAndFeel (&g.getLookAndFeel());
    setUsingNativeTitleBar (true);
    setSize (400, 300);
    auto* const content = new PluginWindowContent (ui, node);
    setContentOwned (content, true);
    
    if (node.isValid())
    {
        setTopLeftPosition (node.getValueTree().getProperty (Tags::windowX, Random::getSystemRandom().nextInt (500)),
                            node.getValueTree().getProperty (Tags::windowY, Random::getSystemRandom().nextInt (500)));
        node.getValueTree().setProperty (Tags::windowVisible, true, 0);
    }
    
	if (auto* ge = dynamic_cast<GenericAudioProcessorEditor*> (ui))
	{
		setResizable (true, true);
	}
    else if (auto* ed = dynamic_cast<AudioProcessorEditor*> (ui))
    {
        setResizable (ed->isResizable(), false);
    }

    setAlwaysOnTop ((bool) node.getProperty (Tags::windowOnTop, false));

    content->stabilizeComponents();
}

PluginWindow::~PluginWindow()
{
    clearContentComponent();
    setLookAndFeel (nullptr);
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
    node.setProperty (Tags::windowX, getX());
    node.setProperty (Tags::windowY, getY());
}

void PluginWindow::closeButtonPressed()
{
    gui.closePluginWindow (this);
}

}
