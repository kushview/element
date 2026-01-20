// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/window.hpp"
#include <element/ui/style.hpp>

namespace element {

using namespace juce;

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

Dialog::~Dialog() {}

void Dialog::closeButtonPressed()
{
    closedSignal();
}

Window::Window (const String& name)
    : DocumentWindow (name, Colors::widgetBackgroundColor, DocumentWindow::closeButton | DocumentWindow::minimiseButton, true)
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

} /* namespace element */
