// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce/core.hpp>
#include <element/juce/gui_basics.hpp>
#include <element/signals.hpp>

namespace element {

class GuiApp;

struct DialogOptions : public juce::DialogWindow::LaunchOptions
{
    DialogOptions()
        : juce::DialogWindow::LaunchOptions()
    {
        dialogBackgroundColour = juce::Colours::darkgrey;
        content.set (nullptr, true);
        dialogTitle = juce::String();
        resizable = false;
        useBottomRightCornerResizer = false;
#if JUCE_IOS
        useNativeTitleBar = false;
#else
        useNativeTitleBar = true;
#endif
        componentToCentreAround = nullptr;
    }
};

/* A mixin for windows/dialogs that provides common signals */
class WindowHook
{
public:
    WindowHook() {}
    virtual ~WindowHook() { juce::Logger::writeToLog ("~WindowHook()"); }
    inline Signal<void()>& signalClosed() { return closedSignal; }

protected:
    Signal<void()> closedSignal;
};

/** A juce DialogWindow that emits a closed signal */
class Dialog : public juce::DialogWindow,
               public WindowHook
{
public:
    Dialog (const juce::String& name);
    virtual ~Dialog();
    virtual void closeButtonPressed();
};

/** A juce DocumentWindow that emits a closed signal */
class Window : public juce::DocumentWindow,
               public WindowHook
{
public:
    Window (const juce::String& name);
    virtual ~Window();
    void closeButtonPressed();
};
} // namespace element
