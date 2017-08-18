
#include "gui/UnlockForm.h"
#include "session/UnlockStatus.h"

struct Spinner  : public Component, private Timer
{
    Spinner()                       { startTimer (1000 / 50); }
    void timerCallback() override   { repaint(); }
    
    void paint (Graphics& g) override
    {
        getLookAndFeel().drawSpinningWaitAnimation (g, Colours::darkgrey, 0, 0, getWidth(), getHeight());
    }
};

struct UnlockForm::OverlayComp  : public Component,
                                  private Thread,
                                  private Timer
{
    OverlayComp (UnlockForm& f, bool d = false)
        : Thread (String()), deactivating(d), form (f)
    {
        result.succeeded = false;
        email = form.emailBox.getText();
        password = form.passwordBox.getText();
        license = form.status.getLicenseKey();
        addAndMakeVisible (spinner);
        startThread (4);
    }
    
    ~OverlayComp()
    {
        stopThread (10000);
    }
    
    void paint (Graphics& g) override
    {
        g.fillAll (Colours::transparentBlack.withAlpha (0.50f));
        
        g.setColour (LookAndFeel_E1::textColor);
        g.setFont (15.0f);
        
        g.drawFittedText (TRANS("Contacting XYZ...").replace ("XYZ", form.status.getWebsiteName()),
                          getLocalBounds().reduced (20, 0).removeFromTop (proportionOfHeight (0.6f)),
                          Justification::centred, 5);
    }
    
    void resized() override
    {
        const int spinnerSize = 40;
        spinner.setBounds ((getWidth() - spinnerSize) / 2, proportionOfHeight (0.6f), spinnerSize, spinnerSize);
    }
    
    void run() override
    {
        if (deactivating)
        {
            result.succeeded = true;
            form.status.deactivateLicense (license);
        }
        else
        {
            result = form.status.attemptWebserverUnlock (email, password);
        }
        
        startTimer (200);
    }
    
    void timerCallback() override
    {
        spinner.setVisible (false);
        stopTimer();
        
        if (result.errorMessage.isNotEmpty())
        {
            AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                              TRANS("Activation Failed"),
                                              result.errorMessage);
        }
        else if (result.informativeMessage.isNotEmpty())
        {
            AlertWindow::showMessageBoxAsync (AlertWindow::InfoIcon,
                                              TRANS("Activation Complete!"),
                                              result.informativeMessage);
        }
        else if (result.urlToLaunch.isNotEmpty())
        {
            URL url (result.urlToLaunch);
            url.launchInDefaultBrowser();
        }
        
        // (local copies because we're about to delete this)
        const bool worked = result.succeeded;
        UnlockForm& f = form;
        
        delete this;
        
        if (worked)
            f.dismiss();
    }
    
    const bool deactivating;
    
    UnlockForm& form;
    Spinner spinner;
    OnlineUnlockStatus::UnlockResult result;
    String email, password, license;
    
    friend class LicenseInfo;
    JUCE_LEAK_DETECTOR (UnlockForm::OverlayComp)
};


namespace Element {
struct LicenseInfo : public Component,
                     public ButtonListener
{
    LicenseInfo (UnlockForm& f) : form(f)
    {
        setOpaque(true);
        addAndMakeVisible(deactivateButton);
        deactivateButton.setButtonText("Deactivate");
        deactivateButton.addListener(this);
        
        addAndMakeVisible(licenseKey);
        licenseKey.setText (String("License: ") + f.status.getLicenseKey(), dontSendNotification);
        licenseKey.setJustificationType (Justification::centred);
        licenseKey.setColour (Label::textColourId, kv::LookAndFeel_E1::textColor);
    }
    
    void paint (Graphics&g) override {
       g.fillAll (LookAndFeel_E1::widgetBackgroundColor);
    }
    
    void resized() override
    {
        Rectangle<int> r (getLocalBounds());
        r.removeFromBottom (80);
        licenseKey.setBounds (r);
        deactivateButton.setSize (90, 30);
        deactivateButton.setBoundsToFit (0, getHeight() - 80, getWidth(), 80, Justification::centred, true);
        
        if (overlay)
            overlay->setBounds (getLocalBounds());
    }
    
    void buttonClicked (Button* b) override
    {
        if (b == &deactivateButton)
            deacviateLicense();
    }
    
private:
    TextButton deactivateButton;
    Label email;
    Label licenseKey;
    UnlockForm& form;
    Component::SafePointer<Component> overlay;
    
    void deacviateLicense()
    {
        if (! overlay)
        {
            addAndMakeVisible (overlay = new UnlockForm::OverlayComp (form, true));
            resized();
        }
    }
    
    JUCE_LEAK_DETECTOR (LicenseInfo);
};

}

static juce_wchar getDefaultPasswordChar() noexcept
{
#if JUCE_LINUX
    return 0x2022;
#else
    return 0x25cf;
#endif
}

UnlockForm::UnlockForm (Element::UnlockStatus& s,
                        const String& userInstructions,
                        bool hasEmailBox,
                        bool hasPasswordBox,
                        bool hasLicenseBox,
                        bool hasCancelButton)
    : message (String(), userInstructions),
      passwordBox (String(), getDefaultPasswordChar()),
      activateButton (TRANS ("Activate")),
      cancelButton (TRANS ("Cancel")),
      status (s),
      useLicense (hasLicenseBox),
      useEmail (hasEmailBox),
      usePassword (hasPasswordBox)
{
    if (usePassword && !useEmail)
    {
        // password requires email too!
        jassertfalse;
        useEmail = hasEmailBox = true;
    }
    
    // Please supply a message to tell your users what to do!
    jassert (userInstructions.isNotEmpty());
    
    setOpaque (true);
    
    message.setJustificationType (Justification::centred);
    
    addAndMakeVisible (message);
    
    if (useEmail)
    {
        addAndMakeVisible (emailBox);
        emailBox.setText (status.getUserEmail());
    }
    
    if (usePassword)
        addAndMakeVisible (passwordBox);
    
    if (useLicense)
    {
        addAndMakeVisible (licenseBox);
        licenseBox.setText (status.getLicenseKey());
    }
    
    if (hasCancelButton)
        addAndMakeVisible (cancelButton);
    
    emailBox.setEscapeAndReturnKeysConsumed (false);
    passwordBox.setEscapeAndReturnKeysConsumed (false);
    licenseBox.setEscapeAndReturnKeysConsumed (false);
    
    addAndMakeVisible (activateButton);
    activateButton.addShortcut (KeyPress (KeyPress::returnKey));
    activateButton.addListener (this);
    cancelButton.addListener (this);
    
    if (status.isUnlocked()) {
        addAndMakeVisible (info = new Element::LicenseInfo (*this));
    }
    
    lookAndFeelChanged();
    setSize (500, 250);
}

UnlockForm::~UnlockForm()
{
    info.deleteAndZero();
    unlockingOverlay.deleteAndZero();
}

void UnlockForm::paint (Graphics& g)
{
    g.fillAll (LookAndFeel_E1::widgetBackgroundColor);
}

void UnlockForm::resized()
{
    /* If you're writing a plugin, then DO NOT USE A POP-UP A DIALOG WINDOW!
       Plugins that create external windows are incredibly annoying for users, and
       cause all sorts of headaches for hosts. Don't be the person who writes that
       plugin that irritates everyone with a nagging dialog box every time they scan!
     */
    jassert (JUCEApplicationBase::isStandaloneApp() || findParentComponentOfClass<DialogWindow>() == nullptr);
    
    const int buttonHeight = 22;
    
    Rectangle<int> r (getLocalBounds().reduced (10, 20));
    
    Rectangle<int> buttonArea (r.removeFromBottom (buttonHeight));
    activateButton.changeWidthToFitText (buttonHeight);
    cancelButton.changeWidthToFitText (buttonHeight);
    
    const int gap = 20;
    buttonArea = buttonArea.withSizeKeepingCentre (activateButton.getWidth()
                                                   + (cancelButton.isVisible() ? gap + cancelButton.getWidth() : 0),
                                                   buttonHeight);
    activateButton.setBounds (buttonArea.removeFromLeft (activateButton.getWidth()));
    buttonArea.removeFromLeft (gap);
    cancelButton.setBounds (buttonArea);
    
    r.removeFromBottom (20);
    
    // (force use of a default system font to make sure it has the password blob character)
    Font font (Font::getDefaultTypefaceForFont (Font (Font::getDefaultSansSerifFontName(),
                                                      Font::getDefaultStyle(),
                                                      5.0f)));
    
    const int boxHeight = 24;
    
    if (useLicense)
    {
        licenseBox.setBounds (r.removeFromBottom (boxHeight));
        licenseBox.setInputRestrictions (512);
        licenseBox.setFont (font);
        r.removeFromBottom (20);
    }
    
    if (usePassword) {
        passwordBox.setBounds (r.removeFromBottom (boxHeight));
        passwordBox.setInputRestrictions (64);
        passwordBox.setFont (font);
        r.removeFromBottom (20);
    }
    
    if (useEmail)
    {
        emailBox.setBounds (r.removeFromBottom (boxHeight));
        emailBox.setInputRestrictions (512);
        emailBox.setFont (font);
    
        r.removeFromBottom (20);
    }
    
    message.setBounds (r);
    
    if (info != nullptr)
        info->setBounds (getLocalBounds());
    
    if (unlockingOverlay != nullptr)
        unlockingOverlay->setBounds (getLocalBounds());
}

void UnlockForm::lookAndFeelChanged()
{
    Colour labelCol (findColour (TextEditor::backgroundColourId).contrasting (0.5f));
    emailBox.setTextToShowWhenEmpty (TRANS ("Email or Username"), labelCol);
    passwordBox.setTextToShowWhenEmpty (TRANS ("Password"), labelCol);
    licenseBox.setTextToShowWhenEmpty (TRANS ("Serial Number"), labelCol);
}

void UnlockForm::showBubbleMessage (const String& text, Component& target)
{
    bubble = new BubbleMessageComponent (500);
    addChildComponent (bubble);
    
    AttributedString attString;
    attString.append (text, Font (16.0f));
    
    bubble->showAt (getLocalArea (&target, target.getLocalBounds()),
                    attString, 500,  // numMillisecondsBeforeRemoving
                    true,  // removeWhenMouseClicked
                    false); // deleteSelfAfterUse
}

void UnlockForm::buttonClicked (Button* b)
{
    if (b == &activateButton)
        attemptRegistration();
    else if (b == &cancelButton)
        dismiss();
}

void UnlockForm::attemptRegistration()
{
    if (unlockingOverlay == nullptr)
    {
        if (useEmail && emailBox.getText().trim().length() < 3)
        {
            showBubbleMessage (TRANS ("Please enter a valid email address!"), emailBox);
            return;
        }
        
        if (usePassword && passwordBox.getText().trim().length() < 3)
        {
            showBubbleMessage (TRANS ("Please enter a valid password key!"), passwordBox);
            return;
        }
        
        if (useLicense && licenseBox.getText().trim().length() < 16)
        {
            showBubbleMessage (TRANS ("Please enter a valid license key!"), licenseBox);
            return;
        }
        
        if (useEmail)
            status.setUserEmail (emailBox.getText().trim());
        if (useLicense)
            status.setLicenseKey (licenseBox.getText().trim());

        addAndMakeVisible (unlockingOverlay = new OverlayComp (*this));
        resized();
        unlockingOverlay->enterModalState();
    }
}

void UnlockForm::dismiss()
{
    status.save();
    if (auto *dw = findParentComponentOfClass<DialogWindow>())
        delete dw;
    else
        delete this;
}
