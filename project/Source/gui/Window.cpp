/*
    Window.cpp - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.

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

#include "GuiApp.h"
#include "Window.h"

namespace Element {
namespace Gui {


    Dialog::Dialog (const String& name, GuiApp& gui_)
        : DialogWindow (name, Colours::darkgrey, true, true),
          gui (gui_)
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


    Window::Window (const String& name, GuiApp& app_)
        : DocumentWindow (name, Colours::darkgrey,
                                DocumentWindow::closeButton |
                                DocumentWindow::minimiseButton,
                                true),
          gui (app_)
    {
#if JUCE_IOS
        setUsingNativeTitleBar (false);
#else
        setUsingNativeTitleBar (true);
#endif
        setResizable (true, false);
    }

    Window::~Window() {}

    void
    Window::closeButtonPressed()
    {
        closedSignal();
    }

}}  /* namespace Element::gui */
