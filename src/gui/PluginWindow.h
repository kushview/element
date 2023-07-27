// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "ElementApp.h"
#include <element/node.hpp>

namespace element {

class Content;
class GuiService;
class Processor;

/** A desktop window containing a plugin's UI. */
class PluginWindow : public DocumentWindow,
                     private Value::Listener
{
public:
    struct Settings
    {
        Colour backgroundColor;
        int titleBarHeight;
    };

    ~PluginWindow();

    float getDesktopScaleFactor() const override;

    Content* getElementContentComponent() const;

    Toolbar* getToolbar() const;
    void updateGraphNode (Processor* newNode, Component* newEditor);
    Node getNode() const { return node; }
    void restoreAlwaysOnTopState();
    void moved() override;
    void closeButtonPressed() override;
    void resized() override;

    void activeWindowStatusChanged() override;

    int getDesktopWindowStyleFlags() const override
    {
        return DocumentWindow::getDesktopWindowStyleFlags() | ComponentPeer::windowHasCloseButton | ComponentPeer::windowHasTitleBar | ComponentPeer::windowHasDropShadow;
    }

protected:
    PluginWindow (GuiService&, Component* const uiComp, const Node& node);

private:
    GuiService& gui;
    friend class WindowManager;
    Processor* owner;
    Node node;
    Value name;

    struct DelayedNodeFocus : public Timer
    {
        DelayedNodeFocus (PluginWindow& w) : window (w) {}
        void timerCallback() override;
        void trigger (int millis = 100) { startTimer (millis); }

        PluginWindow& window;
    } delayedNodeFocus;

    void valueChanged (Value& value) override
    {
        if (value.refersToSameSourceAs (name))
            setName (node.getDisplayName());
    }
};

} // namespace element
