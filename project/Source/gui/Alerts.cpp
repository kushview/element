/*
    Alerts.cpp - This file is part of Element
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


#include "Alerts.h"
#include "alerts/InfoAlertButtonBar.h"
#include "alerts/OkCancelAlertButtonBar.h"
#include "alerts/YesNoCancelButtonBar.h"

namespace Element {
namespace Alerts {

    static const int alertWidth  = 531;
    static const int alertHeight = 391;
    static const int alertBorder = 2;

    void prepareAlert (AlertWindow& alert, Component* buttons)
    {
        assert (nullptr != buttons);

        alert.addCustomComponent (buttons);

        alert.setSize (alertWidth, alertHeight);

        // Position buttons at bottom and along right side.
        // Bg graphic has a 2 pixel border hence the +1
        // pixel border
        buttons->setTopLeftPosition(alert.getWidth() - (buttons->getWidth() + alertBorder + 1),
                                    alert.getHeight() - (buttons->getHeight() + alertBorder));
    }


    /** Create the alert window, and the ButtonBarType which must be a type of
        juce::Component, prepare the window, and run the modal loop for the window.
        @return The value for the button the user clicks. */
    template <class ButtonBarType>
    int showAlert (const String& title, const String& message,
                   AlertWindow::AlertIconType type)
    {
        AlertWindow alert (title, message, type);

        ButtonBarType buttons;

        prepareAlert (alert, &buttons);

        const int result = alert.runModalLoop();
        return result;
    }


    int showInfoAlert (const String& title, const String& message,
                      AlertWindow::AlertIconType type /*=InfoIcon*/)
    {
        return showAlert<Gui::AInfoAlertButtonBar> (title, message, type);
    }


    int showOkCancelAlert (const String& title, const String& message,
                           AlertWindow::AlertIconType type)
    {
        return showAlert<Gui::AOkCancelAlertButtonBar> (title, message, type);
    }


    int showYesNoCancel (const String& title, const String& message,
                             AlertWindow::AlertIconType type)
    {
        return showAlert<Gui::AYesNoCancelButtonBar> (title, message, type);
    }

}}
