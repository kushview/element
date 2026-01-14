// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ElementApp.h"
#include <element/signals.hpp>

namespace element {

class GuiApp;

struct DialogOptions : public DialogWindow::LaunchOptions
{
    DialogOptions()
        : DialogWindow::LaunchOptions()
    {
        dialogBackgroundColour = Colours::darkgrey;
        content.set (nullptr, true);
        dialogTitle = String();
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
    virtual ~WindowHook() { Logger::writeToLog ("~WindowHook()"); }
    inline Signal<void()>& signalClosed() { return closedSignal; }

protected:
    Signal<void()> closedSignal;
};

/** A juce DialogWindow that emits a closed signal */
class Dialog : public DialogWindow,
               public WindowHook
{
public:
    Dialog (const String& name);
    virtual ~Dialog();
    virtual void closeButtonPressed();
};

/** A juce DocumentWindow that emits a closed signal */
class Window : public DocumentWindow,
               public WindowHook
{
public:
    Window (const String& name);
    virtual ~Window();
    void closeButtonPressed();
};
} // namespace element
