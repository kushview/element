
#pragma once

#include "ElementApp.h"

namespace Element {
	class UnlockStatus;
	struct LicenseInfo;
    class Globals;
}

class UnlockForm  : public Component,
                    private Button::Listener
{
public:
    /** Creates an unlock form that will work with the given status object.
        The userInstructions will be displayed above the email and password boxes.
     */
    UnlockForm (Element::Globals&,
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
    
    /** @internal */
    void paint (Graphics&) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void lookAndFeelChanged() override;
    
    Label message;
    TextEditor emailBox, passwordBox, licenseBox;
    TextButton activateButton, cancelButton, deactivateButton, refreshButton;
   
private:
    Element::Globals& world;
    Element::UnlockStatus& status;
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
