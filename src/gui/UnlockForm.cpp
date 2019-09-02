#if 0
#include "controllers/GuiController.h"
#include "controllers/DevicesController.h"
#include "controllers/MappingController.h"
#include "controllers/EngineController.h"
#include "gui/ContentComponent.h"
#include "gui/UnlockForm.h"
#include "gui/ViewHelpers.h"
#include "session/UnlockStatus.h"
#include "session/DeviceManager.h"
#include "Globals.h"

#if EL_RUNNING_AS_PLUGIN
 #include "../../plugins/Element/Source/PluginEditor.h"
#endif

namespace Element 
{

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
    
    OverlayComp (UnlockForm& f, Element::Globals& w, Action a, bool da)
        : Thread (String()), action(a), form (f), world(w), deactivateOthers (da)
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
    
    void clearLicense()
    {
        form.status.saveState (String());
        form.status.loadAll();
    }

    void run() override
    {
        clearLicense();
        StringPairArray params;
        #if defined (EL_PRO)
         params.set ("price_id", String (EL_PRO_PRICE_ID));
        #elif defined (EL_SOLO)
         params.set ("price_id", String (EL_SOLO_PRICE_ID));
        #elif defined (EL_FREE)
         params.set ("price_id", String (EL_LITE_PRICE_ID));
        #endif
        switch (action)
        {
            case Deactivate:
            {
                result = form.status.deactivateLicense (license);
            } break;
            
            case Activate:
            {
                if (deactivateOthers)
                    params.set ("deactivate_others", "1");
                result = form.status.activateLicense (license, {}, {}, params);
            } break;

            case Check:
            {
                result = form.status.checkLicense (license, params);
            } break;
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
       
       #if EL_RUNNING_AS_PLUGIN
        // must happen before deleting 'this'
        typedef ElementPluginAudioProcessorEditor EdType;
        if (EdType* editor = f.findParentComponentOfClass<EdType>())
            editor->triggerAsyncUpdate();
       #endif

        delete this;
        
        if (worked && (a == Activate || a == Check))
        {
            f.saveStatus();
            f.setMode (Deactivate);
        }
        else if (worked && a == Deactivate)
        {
            f.status.saveState (String());
            f.status.loadAll();
            f.setMode (Activate);
        }
        else if (! worked && (a == Activate || a == Check) && f.status.isExpiring())
        {
            f.saveStatus();
            f.setMode (Deactivate);
        }

        if (auto* devs = gui.findSibling<Element::DevicesController>())
            devs->refresh();
        if (auto* maps = gui.findSibling<Element::MappingController>())
            maps->learn (false);
        if (auto* maps = gui.findSibling<Element::MappingController>())
            maps->learn (false);
        if (auto* engine = gui.findSibling<Element::EngineController>())
            engine->sessionReloaded();
        gui.stabilizeContent();
        gui.stabilizeViews();
        
       #if ! EL_RUNNING_AS_PLUGIN
        g.getDeviceManager().restartLastAudioDevice();
       #endif
    }
    
    const Action action;
    UnlockForm& form;
    Element::Globals& world;
    bool deactivateOthers = false;
    Spinner spinner;
    OnlineUnlockStatus::UnlockResult result;
    String email, password, license;
    
    friend struct LicenseInfo;
    JUCE_LEAK_DETECTOR (UnlockForm::OverlayComp)
};

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

        addAndMakeVisible (clearButton);
        clearButton.setButtonText ("Clear");
        clearButton.addListener (this);
        // not supported
        // addAndMakeVisible (forceDeactivate);
        // forceDeactivate.setButtonText ("Force deactivation?");
        // forceDeactivate.setTooltip ("Checking this, if deactivating, will force deactivate all Machine IDs on the server.");

        if (f.status.isExpiring() && Time::getCurrentTime() > f.status.getExpiryTime())
        {
            addAndMakeVisible (expiredNotice);
            String text = (bool)f.status.isTrial() ? "Trial " : "";
            text << "Expired: " << f.status.getExpiryTime().toString(true, false);
            expiredNotice.setText (text, dontSendNotification);
            expiredNotice.setJustificationType (Justification::centred);
            expiredNotice.setColour (Label::textColourId, Colours::red);
        }
        else if (f.status.isExpiring() && Time::getCurrentTime() <= f.status.getExpiryTime())
        {
            addAndMakeVisible (expiredNotice);
            String text = (bool)f.status.isTrial() ? "Trial " : "";
            text << "Expires: " << f.status.getExpiryTime().toString(true, false);
            expiredNotice.setText (text, dontSendNotification);
            expiredNotice.setJustificationType (Justification::centred);
        }
    }
    
    void paint (Graphics&g) override {
       g.fillAll (LookAndFeel_KV1::widgetBackgroundColor);
    }
    
    void resized() override
    {
        Rectangle<int> r (getLocalBounds());
        r.removeFromBottom (80);
        if (expiredNotice.isVisible())
            expiredNotice.setBounds (r.removeFromBottom (14));
        licenseKey.setBounds (r);
        
        refreshButton.setBounds ((getWidth() / 2) - 92 - 23, getHeight() - 50, 90, 24);
        deactivateButton.setBounds ((getWidth() / 2) + 2 - 23, getHeight() - 50, 90, 24);
        clearButton.setBounds (deactivateButton.getRight() + 4, getHeight() - 50, 46, 24);

        // not supported yet
        // const auto refreshX = refreshButton.getBoundsInParent().getX();
        // forceDeactivate.setBounds (refreshX, refreshButton.getBottom() + 2,
        //                            deactivateButton.getRight() - refreshX, 20);

        if (overlay)
            overlay->setBounds (getLocalBounds());
    }
    
    void buttonClicked (Button* b) override
    {
        if (b == &clearButton ||
            (b == &deactivateButton && form.status.isExpiring() && Time::getCurrentTime() > form.status.getExpiryTime()))
        {
            // server won't allow deactivating an expired license, so clear it
            auto& f (form);
            f.status.saveState (String());
            f.status.loadAll();
            f.setMode (Overlay::Activate);
            return;
        }

        if (b == &deactivateButton)
            deacviateLicense();
        else if (b == &refreshButton)
            refreshLicense();
    }
    
private:
    typedef UnlockForm::OverlayComp Overlay;
    TextButton deactivateButton, refreshButton, clearButton;
    Label email;
    Label licenseKey;
    Label expiredNotice;
    ToggleButton forceDeactivate;
    UnlockForm& form;
    Component::SafePointer<Component> overlay;
    
    void deacviateLicense()
    {
        if (overlay)
            return;
        addAndMakeVisible (overlay = new Overlay (form, form.world, Overlay::Deactivate,
                           false));
        resized();
    }
    
    void refreshLicense()
    {
        if (overlay)
            return;
        addAndMakeVisible (overlay = new Overlay (form, form.world, Overlay::Check, false));
        resized();
    }
    
    JUCE_LEAK_DETECTOR (LicenseInfo);
};


static juce_wchar getDefaultPasswordChar() noexcept
{
#if JUCE_LINUX
    return 0x2022;
#else
    return 0x25cf;
#endif
}

UnlockForm::UnlockForm (Globals& s, GuiController& g,
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
    
    addAndMakeVisible (deactivateOthers);
    deactivateOthers.setButtonText ("Deactivate other machines?");
    deactivateOthers.setTooltip ("Checking this will force all other Machine IDs to be cleared from the server before activating this one.");
    
    if (status.isFullVersion() || (status.isExpiring() && Time::getCurrentTime() > status.getExpiryTime())) {
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
    
    r.removeFromBottom (6);
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

    // not yet supported
    const auto activateBtnX = activateButton.getBoundsInParent().getX();
    deactivateOthers.setBounds ({ 
        activateBtnX, activateButton.getBottom() + 2,
        cancelButton.getRight() - activateBtnX, 20 });

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

        addAndMakeVisible (unlockingOverlay = new OverlayComp (
            *this, world, OverlayComp::Activate, deactivateOthers.getToggleState()));
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
    else if (mode == OverlayComp::Deactivate)
    {
        info.deleteAndZero();
        if (status.isUnlocked() || status.isExpiring())
            addAndMakeVisible (info = new Element::LicenseInfo (*this));
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
        dw->closeButtonPressed();
    else
        delete this;
}

}
#endif
