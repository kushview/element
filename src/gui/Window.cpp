/*
    Window.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#include "gui/Window.h"

namespace Element {

    Dialog::Dialog (const String& name)
        : DialogWindow (name, Colours::darkgrey, true, true)
    {
#if JUCE_IOS
        setUsingNativeTitleBar (false);
#else
        setUsingNativeTitleBar (true);
#endif
        setResizable (true, false);
    }

    Dialog::~Dialog() { }

    void
    Dialog::closeButtonPressed()
    {
        closedSignal();
    }


    Window::Window (const String& name)
        : DocumentWindow (name, LookAndFeel_KV1::widgetBackgroundColor,
                          DocumentWindow::closeButton | DocumentWindow::minimiseButton, true)
    {
       #if JUCE_IOS
        setUsingNativeTitleBar (false);
       #else
        setUsingNativeTitleBar (true);
       #endif
        setResizable (true, false);
    }

    Window::~Window() {}

    void Window::closeButtonPressed()
    {
        closedSignal();
    }

}  /* namespace Element */
