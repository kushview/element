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

#if 0

#pragma once

#include "ElementApp.h"

namespace Element {
	class UnlockStatus;
	struct LicenseInfo;
    class Globals;
    class GuiController;


class UnlockForm  : public Component,
                    private Button::Listener
{
public:
    /** Creates an unlock form that will work with the given status object.
        The userInstructions will be displayed above the email and password boxes.
     */
    UnlockForm (Element::Globals&, Element::GuiController&,
                const String& userInstructions,
                bool hasEmailBox = true,
                bool hasPasswordBox = true,
                bool hasLicenseBox = false,
                bool hasCancelButton = true);
    
    /** Destructor. */
    ~UnlockForm();
    
    /** This is called when the form is dismissed (either cancelled or when registration
        succeeds).
        By default it will delete this, but you can override it to do other things.
     */
    virtual void dismiss();
    
    void setMode (int);
    
    void saveStatus();
    
    /** @internal */
    void paint (Graphics&) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void lookAndFeelChanged() override;
    
    Label message;
    TextEditor emailBox, passwordBox, licenseBox;
    TextButton activateButton, cancelButton, deactivateButton, refreshButton;
    ToggleButton deactivateOthers;
     
private:
    Element::Globals& world;
    Element::UnlockStatus& status;
    Element::GuiController& gui;
    bool useLicense, useEmail, usePassword;
    
    ScopedPointer<BubbleMessageComponent> bubble;
    
    struct OverlayComp;
    friend struct OverlayComp;
    Component::SafePointer<Component> unlockingOverlay;
    
    friend struct Element::LicenseInfo;
    Component::SafePointer<Component> info;
    
    void buttonClicked (Button*) override;
    void attemptRegistration();
    void showBubbleMessage (const String&, Component&);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UnlockForm)
};

}
#endif
