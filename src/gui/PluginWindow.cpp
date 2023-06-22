/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

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

#include <element/services/guiservice.hpp>
#include <element/nodeobject.hpp>
#include "gui/GuiCommon.h"
#include "gui/PluginWindow.h"
#include "gui/ContextMenus.h"
#include "gui/nodes/VolumeNodeEditor.h"
#include "session/presetmanager.hpp"

namespace element {
static Array<PluginWindow*> activePluginWindows;

class PluginWindowToolbar : public Toolbar
{
public:
    enum Items
    {
        BypassPlugin = 1
    };

    PluginWindowToolbar() {}
    ~PluginWindowToolbar() {}
};

class PluginWindowContent : public Component,
                            public ComponentListener,
                            public Button::Listener
{
public:
    PluginWindowContent (Component* const _editor, const Node& _node)
        : editor (_editor), object (_node.getObject()), node (_node)
    {
        nativeEditor = nullptr != dynamic_cast<AudioProcessorEditor*> (_editor) && nullptr == dynamic_cast<GenericAudioProcessorEditor*> (_editor);

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
        powerButton.getToggleStateValue().referTo (node.getPropertyAsValue (tags::bypass));
        powerButton.setClickingTogglesState (true);
        powerButton.addListener (this);

        addAndMakeVisible (onTopButton);
        onTopButton.setButtonText ("^");
        onTopButton.setTooltip ("Keep plugin window on top of others");
        onTopButton.addListener (this);

        addAndMakeVisible (muteButton);
        muteButton.setYesNoText ("M", "M");
        muteButton.setColour (SettingButton::backgroundOnColourId, Colors::toggleRed);
        muteButton.getToggleStateValue().referTo (node.getPropertyAsValue (tags::mute));
        muteButton.setClickingTogglesState (true);
        muteButton.addListener (this);

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

        editor = nullptr;
        toolbar = nullptr;
        leftPanel = nullptr;
        rightPanel = nullptr;
    }

    void updateSize()
    {
        const int height = jmax (editor->getHeight(), 100) + toolbar->getHeight();
        setSize (editor->getWidth(), height + 4);
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

        if (nativeEditor)
        {
            editor->setBounds (0, r.getY(), editor->getWidth(), editor->getHeight());
        }
        else
        {
            editor->setBounds (0, r.getY(), getWidth(), getHeight() - r.getY());
        }

        editor->addComponentListener (this);
    }

    void buttonClicked (Button* button) override
    {
        if (button == &powerButton)
        {
            if (object && object->isSuspended() != node.isBypassed())
                object->suspendProcessing (node.isBypassed());
        }
        else if (button == &nodeButton)
        {
            auto* const world = ViewHelpers::getGlobals (this);
            auto* callback = new MenuCallback (this, node);
            NodePopupMenu& menu (callback->menu);
            menu.addSeparator();
            menu.addOptionsSubmenu();
            if (world)
                menu.addPresetsMenu (world->presets());
            menu.show (0, 0, 0, 0, callback);
        }
        else if (button == &onTopButton)
        {
            if (auto* pw = findParentComponentOfClass<PluginWindow>())
            {
                pw->setAlwaysOnTop (! pw->isAlwaysOnTop());
                node.setProperty (tags::windowOnTop, pw->isAlwaysOnTop());
            }
        }
        else if (button == &muteButton)
        {
            node.setMuted (muteButton.getToggleState());
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

    void handleMenuResult (int result)
    {
        // noop, might need when the menu gets more complex
    }

private:
    JUCE_DECLARE_WEAK_REFERENCEABLE (PluginWindowContent);
    ScopedPointer<PluginWindowToolbar> toolbar;
    SettingButton nodeButton;
    PowerButton powerButton;
    SettingButton onTopButton;
    SettingButton muteButton;
    Value bypassValue;
    bool nativeEditor = false;
    ScopedPointer<Component> editor, leftPanel, rightPanel;
    NodeObjectPtr object;
    Node node;

    class MenuCallback : public ModalComponentManager::Callback
    {
    public:
        MenuCallback (PluginWindowContent* c, const Node& n)
            : content (c), menu (n)
        {
        }

        void modalStateFinished (int returnValue) override
        {
            if (! content.wasObjectDeleted())
                if (auto* const msg = menu.createMessageForResultCode (returnValue))
                    ViewHelpers::postMessageFor (content.get(), msg);
        }

        WeakReference<PluginWindowContent> content;
        NodePopupMenu menu;
    };

    AudioProcessor* getProcessor() { return (object != nullptr) ? object->getAudioProcessor() : nullptr; }
};

void PluginWindow::DelayedNodeFocus::timerCallback()
{
    if (! window.isActiveWindow())
        return;
    if (auto* const cc = ViewHelpers::findContentComponent())
    {
        auto node = window.getNode();
        if (node.isValid())
            if (auto* const gui = cc->services().find<GuiService>())
                gui->selectNode (node);
    }
}

PluginWindow::PluginWindow (GuiService& g, Component* const ui, const Node& n)
    : DocumentWindow (n.getName(), LookAndFeel::backgroundColor, DocumentWindow::minimiseButton | DocumentWindow::closeButton, false),
      gui (g),
      owner (n.getObject()),
      node (n),
      delayedNodeFocus (*this)
{
    setLookAndFeel (&g.getLookAndFeel());
    setUsingNativeTitleBar (true);
    setSize (400, 300);

    name = node.getPropertyAsValue (tags::name);
    name.addListener (this);
    setName (node.getDisplayName());

    if (node.isValid())
    {
        setTopLeftPosition (node.data().getProperty (tags::windowX, Random::getSystemRandom().nextInt (500)),
                            node.data().getProperty (tags::windowY, Random::getSystemRandom().nextInt (500)));
        node.data().setProperty (tags::windowVisible, true, 0);
    }

    bool windowResize = false;
    bool useResizeHandle = false;

    if (nullptr != dynamic_cast<GenericAudioProcessorEditor*> (ui))
    {
        setResizable (false, false);
    }
    else if (auto* ed = dynamic_cast<AudioProcessorEditor*> (ui))
    {
        windowResize = false;
        useResizeHandle = ed->isResizable() && ed->resizableCorner == nullptr;
    }
    else if (nullptr != dynamic_cast<VolumeNodeEditor*> (ui))
    {
        windowResize = false;
        useResizeHandle = false;
    }
    else
    {
        windowResize = true;
        useResizeHandle = false;
    }

    setResizable (windowResize, useResizeHandle);

    const bool defaultOnTop = g.context().settings().pluginWindowsOnTop();
    setAlwaysOnTop ((bool) node.getProperty (tags::windowOnTop, defaultOnTop));

    auto* const content = new PluginWindowContent (ui, node);
    setContentOwned (content, true);

    addToDesktop();
    content->stabilizeComponents();
}

PluginWindow::~PluginWindow()
{
    delayedNodeFocus.stopTimer();
    name.removeListener (this);
    clearContentComponent();
    setLookAndFeel (nullptr);
}

float PluginWindow::getDesktopScaleFactor() const { return 1.f; }

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

void PluginWindow::restoreAlwaysOnTopState()
{
    if (node.isValid())
    {
        const auto shouldBeOnTop = (bool) node.getProperty (tags::windowOnTop);
        setAlwaysOnTop (shouldBeOnTop);
        if (shouldBeOnTop)
            toFront (false);
    }
}

void PluginWindow::resized()
{
    DocumentWindow::resized();
    if (getWidth() <= 140)
    {
        setName ({});
    }
    else
    {
        setName (node.getDisplayName());
    }
}

void PluginWindow::activeWindowStatusChanged()
{
    if (isActiveWindow())
        delayedNodeFocus.trigger();

    if (nullptr == getContentComponent() || isActiveWindow())
        return;
    gui.checkForegroundStatus();
}

void PluginWindow::updateGraphNode (NodeObject* newNode, Component* newEditor)
{
    jassert (nullptr != newNode && nullptr != newEditor);
    owner = newNode;
    setContentOwned (newEditor, true);
}

void PluginWindow::moved()
{
    node.setProperty (tags::windowX, getX());
    node.setProperty (tags::windowY, getY());
}

void PluginWindow::closeButtonPressed()
{
    gui.closePluginWindow (this);
}

} // namespace element
