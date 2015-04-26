/*
    Alerts.h - This file is part of Element
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

#ifndef ELEMENT_GUI_ALERTS_H
#define ELEMENT_GUI_ALERTS_H

#include <element/Juce.h>

namespace Element {

/** These alerts are customizations of juce's Alert window. They behave the
    mostly the same way, but have a look appropriate for BTV.

    See juce_AlertWindow.h, AlertBackground.h, InfoAlertButtonBar.h,
    OkCancelAlertButtonBar.h, YesNoCancelButtonBar.h, and
    see ALookAndFeel::drawAlertBox(). */
namespace Alerts
{
    /** Show alert with Beat Thang background by default and only an Ok button.
        \param type Determines the background image used in the dialog. May be
                    any of juce::AlertWindow values, but NoIcon is not honored.
        \return Always returns 0. */
    int showInfoAlert (const String& title, const String& message,
                       AlertWindow::AlertIconType type = AlertWindow::InfoIcon);

    /** Show alert with an exlamation point background by default and Ok and Cancel buttons.
     *  \param type Determines the background image used in the dialog. May be
     *              any of juce::AlertWindow values, but NoIcon is not honored.
     *  \returns One of OkCancelChoices.
     */
    int showOkCancelAlert (const String& title, const String& message,
                           AlertWindow::AlertIconType type = AlertWindow::WarningIcon);
    enum OkCancelChoices { kOcCancel, kOcOkay };

    /** Show alert with question mark background by default and Yes, No and Cancel buttons.
     *  \param type Determines the background image used in the dialog. May be
     *              any of juce::AlertWindow values, but NoIcon is not honored.
     *  \returns One of YesNoCancelChoices.
     */
    int showYesNoCancel (const String& title, const String& message,
                              AlertWindow::AlertIconType type = AlertWindow::QuestionIcon);
    enum YesNoCancelChoices { kYncCancel, kYncYes, kYncNo };

}}  /* namespace Element::Alerts */

#endif  /* ELEMENT_GUI_ALERTS_H */
