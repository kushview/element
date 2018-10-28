#include "controllers/GuiController.h"
#include "controllers/DevicesController.h"
#include "controllers/MappingController.h"
#include "gui/UnlockForm.h"
#include "session/UnlockStatus.h"
#include "session/DeviceManager.h"
#include "Globals.h"

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
    enum Action {
        Activate, Deactivate, Check
    };
    
    OverlayComp (UnlockForm& f, Element::Globals& w, Action a)
        : Thread (String()), action(a), form (f), world(w)
    {
        result.succeeded = false;
        email       = form.emailBox.getText();
        password    = form.passwordBox.getText();
        license     = form.status.getLicenseKey();
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
        
        g.setColour (LookAndFeel_KV1::textColor);
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
        switch (action)
        {
            case Deactivate:
                result = form.status.deactivateLicense (license);
                break;
            case Activate:
                result = form.status.activateLicense (license);
                break;
            case Check:
                result = form.status.checkLicense (license);
                break;
        }
        
        startTimer (200);
    }
    
    String getActionTitle (bool failed) const
    {
        String title;
        switch (action)
        {
            case Activate:      title = "Activation"; break;
            case Deactivate:    title = "Deactivation"; break;
            case Check:         title = "License Check"; break;
        }
        
        title << (failed ? " Failed" : " Complete");
        return TRANS (title);
    }
    
    void timerCallback() override
    {
        spinner.setVisible (false);
        stopTimer();
        
        if (result.errorMessage.isNotEmpty())
        {
            AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                              getActionTitle (true),
                                              result.errorMessage);
        }
        else if (result.informativeMessage.isNotEmpty())
        {
            AlertWindow::showMessageBoxAsync (AlertWindow::InfoIcon,
                                              getActionTitle (false),
                                              result.informativeMessage);
        }
        else if (result.urlToLaunch.isNotEmpty())
        {
            URL url (result.urlToLaunch);
            url.launchInDefaultBrowser();
        }
        
        // (local copies because we're about to delete this)
        const bool worked = result.succeeded;
        auto& g = world;
        const auto a = action;
        UnlockForm& f = form;
        auto& gui = f.gui;
        
        delete this;
        
        if (worked && (a == Activate || a == Check))
        {
            f.setMode (Deactivate);
            f.saveStatus();
            g.getDeviceManager().restartLastAudioDevice();
        }
        else if (worked && a == Deactivate)
        {
            f.setMode (Activate);
        }

        if (auto* devs = gui.findSibling<Element::DevicesController>())
            devs->refresh();
        if (auto* maps = gui.findSibling<Element::MappingController>())
            maps->learn (false);
        
        gui.stabilizeContent();
        gui.stabilizeViews();
    }
    
    const Action action;
    UnlockForm& form;
    Element::Globals& world;
    Spinner spinner;
    OnlineUnlockStatus::UnlockResult result;
    String email, password, license;
    
    friend class LicenseInfo;
    JUCE_LEAK_DETECTOR (UnlockForm::OverlayComp)
};

namespace Element {
struct LicenseInfo : public Component,
                     public Button::Listener
{
    LicenseInfo (UnlockForm& f) : form(f)
    {
        setOpaque(true);
        
        addAndMakeVisible (licenseKey);
        licenseKey.setText (String("License: ") + f.status.getLicenseKey(), dontSendNotification);
        licenseKey.setJustificationType (Justification::centred);
        licenseKey.setColour (Label::textColourId, kv::LookAndFeel_KV1::textColor);
        
        addAndMakeVisible (refreshButton);
        refreshButton.setButtonText ("Refresh");
        refreshButton.addListener (this);
        
        addAndMakeVisible (deactivateButton);
        deactivateButton.setButtonText ("Deactivate");
        deactivateButton.addListener (this);
    }
    
    void paint (Graphics&g) override {
       g.fillAll (LookAndFeel_KV1::widgetBackgroundColor);
    }
    
    void resized() override
    {
        Rectangle<int> r (getLocalBounds());
        r.removeFromBottom (80);
        licenseKey.setBounds (r);
        
        refreshButton.setBounds ((getWidth() / 2) - 92, getHeight() - 40, 90, 30);
        deactivateButton.setBounds ((getWidth() / 2) + 2, getHeight() - 40, 90, 30);
        
        if (overlay)
            overlay->setBounds (getLocalBounds());
    }
    
    void buttonClicked (Button* b) override
    {
        if (b == &deactivateButton)
            deacviateLicense();
        else if (b == &refreshButton)
            refreshLicense();
    }
    
private:
    typedef UnlockForm::OverlayComp Overlay;
    TextButton deactivateButton, refreshButton;
    Label email;
    Label licenseKey;
    UnlockForm& form;
    Component::SafePointer<Component> overlay;
    
    void deacviateLicense()
    {
        if (overlay)
            return;
        addAndMakeVisible (overlay = new Overlay (form, form.world, Overlay::Deactivate));
        resized();
    }
    
    void refreshLicense()
    {
        if (overlay)
            return;
        addAndMakeVisible (overlay = new Overlay (form, form.world, Overlay::Check));
        resized();
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

UnlockForm::UnlockForm (Element::Globals& s, Element::GuiController& g,
                        const String& userInstructions,
                        bool hasEmailBox,
                        bool hasPasswordBox,
                        bool hasLicenseBox,
                        bool hasCancelButton)
    : message (String(), userInstructions),
      passwordBox (String(), getDefaultPasswordChar()),
      activateButton (TRANS ("Activate")),
      cancelButton (TRANS ("Cancel")),
      world (s),
      status (s.getUnlockStatus()),
      gui (g),
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
    g.fillAll (LookAndFeel_KV1::widgetBackgroundColor);
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
    message.setBounds (r);

    // (force use of a default system font to make sure it has the password blob character)
    Font font (Font::getDefaultTypefaceForFont (Font (Font::getDefaultSansSerifFontName(),
                                                      Font::getDefaultStyle(),
                                                      5.0f)));
    
    const int boxHeight = 24;
    
    if (useLicense)
    {
        auto r2 = r;
        r2.removeFromTop (r2.getHeight() / 2 + 40);
        licenseBox.setBounds (r2.removeFromTop (boxHeight));
        licenseBox.setInputRestrictions (512);
        licenseBox.setFont (font);
        r2.removeFromBottom (20);
    }

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
    licenseBox.setTextToShowWhenEmpty (TRANS ("License Key"), labelCol);
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

        addAndMakeVisible (unlockingOverlay = new OverlayComp (*this, world, OverlayComp::Activate));
        resized();
        unlockingOverlay->enterModalState();
    }
}

void UnlockForm::setMode (int mode)
{
    if (mode == OverlayComp::Activate && info != nullptr)
    {
        if (auto* c = info.getComponent())
        {
            removeChildComponent (c);
            delete c;
        }
    }
    else if (status.isUnlocked())
    {
        info.deleteAndZero();
        addAndMakeVisible (info = new Element::LicenseInfo (*this));
    }

    resized();
}

void UnlockForm::saveStatus()
{
    status.save();
    status.loadAll();
}

void UnlockForm::dismiss()
{
    saveStatus();
    if (auto *dw = findParentComponentOfClass<DialogWindow>())
        delete dw;
    else
        delete this;
}
