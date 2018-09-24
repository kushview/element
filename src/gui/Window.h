/*
    Window.h - This file is part of Element
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

#ifndef ELEMENT_WINDOW_H
#define ELEMENT_WINDOW_H

#include "ElementApp.h"
#include "Signals.h"

namespace Element {

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
        WindowHook() { }
        virtual ~WindowHook() { Logger::writeToLog("~WindowHook()"); }
        inline Signal& signalClosed() { return closedSignal; }

    protected:
        Signal  closedSignal;
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
}
#endif
