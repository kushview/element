
#pragma once

#include "ElementApp.h"

class UnlockForm  : public Component,
                    private ButtonListener
{
public:
    /** Creates an unlock form that will work with the given status object.
     The userInstructions will be displayed above the email and password boxes.
     */
    UnlockForm (OnlineUnlockStatus&, const String& userInstructions,
                bool hasCancelButton = true);
    
    /** Destructor. */
    ~UnlockForm();
    
    /** This is called when the form is dismissed (either cancelled or when registration
     succeeds).
     By default it will delete this, but you can override it to do other things.
     */
    virtual void dismiss();
    
    /** @internal */
    void paint (Graphics&) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void lookAndFeelChanged() override;
    
    Label message;
    TextEditor emailBox, passwordBox;
    TextButton registerButton, cancelButton;
    
private:
    OnlineUnlockStatus& status;
    ScopedPointer<BubbleMessageComponent> bubble;
    
    struct OverlayComp;
    friend struct OverlayComp;
    Component::SafePointer<Component> unlockingOverlay;
    
    void buttonClicked (Button*) override;
    void attemptRegistration();
    void showBubbleMessage (const String&, Component&);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UnlockForm)
};
