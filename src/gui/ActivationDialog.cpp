/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.4.2

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2017 - ROLI Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
#include "controllers/GuiController.h"
#include "gui/Buttons.h"
#include "URLs.h"

#ifndef EL_ALLOW_TRIAL_REGISTRATION
 #define EL_ALLOW_TRIAL_REGISTRATION 0
#endif
//[/Headers]

#include "ActivationDialog.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
namespace Element {
//[/MiscUserDefs]

//==============================================================================
ActivationComponent::ActivationComponent (GuiController& g)
    : gui(g), progressBar(progress)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    appNameLabel.reset (new Label ("AppNameLabel",
                                   TRANS("ELEMENT")));
    addAndMakeVisible (appNameLabel.get());
    appNameLabel->setExplicitFocusOrder (7);
    appNameLabel->setFont (Font (44.00f, Font::plain).withTypefaceStyle ("Regular"));
    appNameLabel->setJustificationType (Justification::centred);
    appNameLabel->setEditable (false, false, false);
    appNameLabel->setColour (TextEditor::textColourId, Colours::black);
    appNameLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    onlineActivateLink.reset (new HyperlinkButton (TRANS("Online Activation"),
                                                   URL ("EL_URL_HELP_ACTIVATION")));
    addAndMakeVisible (onlineActivateLink.get());
    onlineActivateLink->setTooltip (TRANS("EL_URL_HELP_ACTIVATION"));
    onlineActivateLink->setExplicitFocusOrder (8);
    onlineActivateLink->setButtonText (TRANS("Online Activation"));

    instructionLabel.reset (new Label ("InstructionLabel",
                                       TRANS("APPNAME requires activation to run.  \n"
                                       "Please enter your FULL or TRIAL license key for APPNAME.  ")));
    addAndMakeVisible (instructionLabel.get());
    instructionLabel->setExplicitFocusOrder (9);
    instructionLabel->setFont (Font (14.00f, Font::plain).withTypefaceStyle ("Regular"));
    instructionLabel->setJustificationType (Justification::centred);
    instructionLabel->setEditable (false, false, false);
    instructionLabel->setColour (TextEditor::textColourId, Colours::black);
    instructionLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    licenseKey.reset (new TextEditor ("LicenseKey"));
    addAndMakeVisible (licenseKey.get());
    licenseKey->setExplicitFocusOrder (1);
    licenseKey->setMultiLine (false);
    licenseKey->setReturnKeyStartsNewLine (false);
    licenseKey->setReadOnly (false);
    licenseKey->setScrollbarsShown (true);
    licenseKey->setCaretVisible (true);
    licenseKey->setPopupMenuEnabled (false);
    licenseKey->setText (String());

    activateButton.reset (new TextButton ("ActivateButton"));
    addAndMakeVisible (activateButton.get());
    activateButton->setExplicitFocusOrder (2);
    activateButton->setButtonText (TRANS("Activate"));
    activateButton->addListener (this);

    quitButton.reset (new TextButton ("QuitButton"));
    addAndMakeVisible (quitButton.get());
    quitButton->setExplicitFocusOrder (3);
    quitButton->setButtonText (TRANS("Start Trial"));
    quitButton->addListener (this);

    myLicenseLink.reset (new HyperlinkButton (TRANS("My Licenses"),
                                              URL ("http://www.juce.com")));
    addAndMakeVisible (myLicenseLink.get());
    myLicenseLink->setTooltip (TRANS("http://www.juce.com"));
    myLicenseLink->setExplicitFocusOrder (4);
    myLicenseLink->setButtonText (TRANS("My Licenses"));

    getLicenseLink.reset (new HyperlinkButton (TRANS("Get A License"),
                                               URL ("https://kushview.net/element/purchase")));
    addAndMakeVisible (getLicenseLink.get());
    getLicenseLink->setTooltip (TRANS("https://kushview.net/element/purchase"));
    getLicenseLink->setExplicitFocusOrder (5);
    getLicenseLink->setButtonText (TRANS("Get A License"));

    registerTrialLink.reset (new HyperlinkButton (TRANS("Register For Trial"),
                                                  URL ("https://kushview.net/element/trial")));
    addAndMakeVisible (registerTrialLink.get());
    registerTrialLink->setTooltip (TRANS("https://kushview.net/element/trial"));
    registerTrialLink->setExplicitFocusOrder (6);
    registerTrialLink->setButtonText (TRANS("Register For Trial"));

    instructionLabel2.reset (new Label ("InstructionLabel",
                                        TRANS("Your license key will be emailed to you upon sucessful registration. Fake credentials won\'t cut it.")));
    addAndMakeVisible (instructionLabel2.get());
    instructionLabel2->setExplicitFocusOrder (9);
    instructionLabel2->setFont (Font (14.00f, Font::italic));
    instructionLabel2->setJustificationType (Justification::centred);
    instructionLabel2->setEditable (false, false, false);
    instructionLabel2->setColour (Label::textColourId, Colour (0xfff2f2f2));
    instructionLabel2->setColour (TextEditor::textColourId, Colours::black);
    instructionLabel2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));


    //[UserPreSize]
    appNameLabel->setText (Util::appName().toUpperCase(), dontSendNotification);
    instructionLabel->setText (
        instructionLabel->getText().replace("APPNAME", Util::appName()),
        dontSendNotification);
    licenseKey->setWantsKeyboardFocus (true);
    instructionLabel2->setVisible (false);
    registerTrialLink->setVisible (false);
    onlineActivateLink->setURL (URL (EL_URL_HELP_ACTIVATION));
    myLicenseLink->setURL (URL (EL_URL_MY_LICENSES));
    getLicenseLink->setURL (URL (EL_URL_ELEMENT_PURCHASE));
    registerTrialLink->setURL (URL (EL_URL_ELEMENT_GET_TRIAL));

    password.setPasswordCharacter (Util::defaultPasswordChar());

    syncButton.reset(new IconButton ("Refresh"));
    syncButton->setIcon (Icon (getIcons().farSyncAlt, LookAndFeel::textColor));
    syncButton->addListener (this);

   #if ! EL_ALLOW_TRIAL_REGISTRATION
    quitButton->setButtonText ("Quit");
   #endif
    //[/UserPreSize]

    setSize (480, 346);


    //[Constructor] You can add your own custom stuff here..
    setBackgroundColour (findColour (DocumentWindow::backgroundColourId));
    startTimer (250);
    //[/Constructor]
}

ActivationComponent::~ActivationComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    syncButton->removeListener (this);
    syncButton = nullptr;
    //[/Destructor_pre]

    appNameLabel = nullptr;
    onlineActivateLink = nullptr;
    instructionLabel = nullptr;
    licenseKey = nullptr;
    activateButton = nullptr;
    quitButton = nullptr;
    myLicenseLink = nullptr;
    getLicenseLink = nullptr;
    registerTrialLink = nullptr;
    instructionLabel2 = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void ActivationComponent::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xff323e44));

    //[UserPaint] Add your own custom painting code here..
    g.fillAll (backgroundColour);
    //[/UserPaint]
}

void ActivationComponent::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    appNameLabel->setBounds ((getWidth() / 2) - (320 / 2), 37, 320, 48);
    onlineActivateLink->setBounds ((getWidth() / 2) - (108 / 2), 94, 108, 18);
    instructionLabel->setBounds ((getWidth() / 2) - (352 / 2), 124, 352, 39);
    licenseKey->setBounds ((getWidth() / 2) - (260 / 2), 171, 260, 22);
    activateButton->setBounds (((getWidth() / 2) - (260 / 2)) + 260 / 2 + -3 - 90, 208, 90, 24);
    quitButton->setBounds (((getWidth() / 2) - (260 / 2)) + 260 / 2 + 3, 208, 90, 24);
    myLicenseLink->setBounds ((getWidth() / 2) - (96 / 2), 249, 96, 18);
    getLicenseLink->setBounds ((getWidth() / 2) - (94 / 2), 272, 94, 18);
    registerTrialLink->setBounds ((getWidth() / 2) - (100 / 2), 293, 100, 18);
    instructionLabel2->setBounds ((getWidth() / 2) - (280 / 2), 240, 280, 64);
    //[UserResized] Add your own custom resize handling here..

    if (syncButton && syncButton->isVisible())
    {
        int shiftLeft = activateButton->getHeight() / 2;
        activateButton->setBounds (activateButton->getBoundsInParent()
            .withX (activateButton->getX() - shiftLeft));
        quitButton->setBounds (quitButton->getBoundsInParent()
            .withX (quitButton->getX() - shiftLeft));
        syncButton->setBounds (quitButton->getRight() + 6, quitButton->getY(),
                               quitButton->getHeight(), quitButton->getHeight());
    }

    if (isForRegistration)
    {
        instructionLabel->setBounds (
            instructionLabel->getBoundsInParent().withY (
                instructionLabel->getBoundsInParent().getY() - 40));

        Rectangle<int> regBox = {
            licenseKey->getX(),
            instructionLabel->getBottom(),
            licenseKey->getWidth(),
            jmax(24 * 3, quitButton->getY() - instructionLabel->getBottom())
        };

        auto leftSlice = regBox.removeFromLeft (90);

        emailLabel.setBounds (leftSlice.removeFromTop (24));
        userNameLabel.setBounds (leftSlice.removeFromTop (24));
        passwordLabel.setBounds (leftSlice.removeFromTop (24));

        email.setBounds (regBox.removeFromTop (22));
        regBox.removeFromTop (3);
        username.setBounds (regBox.removeFromTop (22));
        regBox.removeFromTop (3);
        password.setBounds (regBox.removeFromTop (22));
    }

    if (progressBar.isVisible())
    {
        progressBar.setBounds (licenseKey->getBounds().expanded (30, 2));
    }

    if (unlock)
        unlock->setBounds (getLocalBounds());
    //[/UserResized]
}

void ActivationComponent::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    auto& status = gui.getWorld().getUnlockStatus();
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == activateButton.get())
    {
        //[UserButtonCode_activateButton] -- add your button handler code here..
        if (isForTrial)
        {
            if (EL_IS_TRIAL_EXPIRED (status))
            {
                URL purchaseUrl (EL_URL_ELEMENT_PURCHASE);
                purchaseUrl.launchInDefaultBrowser();
            }
            else
            {
                URL upgradeUrl (EL_URL_LICENSE_UPGRADE);
                upgradeUrl = upgradeUrl.withParameter("license_id", gui.getWorld().getUnlockStatus().getLicenseKey())
                                    .withParameter("upgrade_id", "1");
                upgradeUrl.launchInDefaultBrowser();
            }
        }
        else if (isForRegistration)
        {
            if (email.isEmpty() || username.isEmpty() || password.isEmpty())
                return;

            if (! URL::isProbablyAnEmailAddress (email.getText()))
            {
                AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                    "Invalid Email", "Please enter a valid email address");
                return;
            }

            if (password.getText().length() < 8)
            {
                AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                    "Password to Short", "Please enter a password with 8 or more characters");
                return;
            }

           #if ! EL_USE_LOCAL_AUTH
            if (Util::isGmailExtended (email.getText()))
            {
                AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                    "Invalid Email", "Gmail extended addresses are not permitted in registration");
                return;
            }
           #endif

            auto* unlockRef = new UnlockOverlay (unlock,
                gui.getWorld().getUnlockStatus(),
                gui.getWorld(),
                UnlockOverlay::Register,
                String(),
                email.getText(),
                username.getText(),
                password.getText()
            );
            unlockRef->setOpacity (0.72f);
            unlockRef->onFinished = [this](const UnlockStatus::UnlockResult result, UnlockOverlay::Action)
            {
                auto& status = gui.getWorld().getUnlockStatus();
                if (result.succeeded)
                {
                    bool shouldBail = false;
                    // AlertWindow::showMessageBoxAsync (AlertWindow::InfoIcon,
                    //                                 "Trial Activation",
                    //                                 result.informativeMessage);
                    if (EL_IS_TRIAL_NOT_EXPIRED(status) ||
                        EL_IS_TRIAL_EXPIRED(status))
                    {
                        setForRegistration (false);
                        isForTrial = false;
                        setForTrial (true);
                    }
                    else if (EL_IS_NOT_ACTIVATED(status))
                    {
                        setForRegistration (false);
                    }
                    else
                    {
                        setForRegistration (false);
                        if (auto* dialog = findParentComponentOfClass<ActivationDialog>()) {
                            dialog->closeButtonPressed();
                            shouldBail = true;
                        }
                    }
                    status.refreshed();
                    if (shouldBail)
                        return;
                }
                else
                {
                    // AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                    //                                 "Trial Activation",
                    //                                 result.errorMessage);
                }
            };
            addAndMakeVisible (unlock.get());
        }
        else
        {
            if (licenseKey->isEmpty())
                return;
            auto* unlockRef = new UnlockOverlay (unlock,
                status, gui.getWorld(), UnlockOverlay::Activate,
                licenseKey->getText().trim()
            );
            unlockRef->setOpacity (0.72f);
            unlockRef->onFinished = [this](const UnlockStatus::UnlockResult result, UnlockOverlay::Action)
            {
                if (result.succeeded)
                {
                    auto& _status = gui.getWorld().getUnlockStatus();
                    if (EL_IS_TRIAL_EXPIRED(_status) ||
                        EL_IS_TRIAL_NOT_EXPIRED(_status))
                    {
                        isForTrial = false;
                        setForTrial (true);
                        resized();
                    }
                    else if (auto* dialog = findParentComponentOfClass<ActivationDialog>())
                    {
                        dialog->closeButtonPressed();
                    }
                    _status.refreshed();
                }
            };
            addAndMakeVisible (unlock.get());
        }

        resized();
        //[/UserButtonCode_activateButton]
    }
    else if (buttonThatWasClicked == quitButton.get())
    {
        //[UserButtonCode_quitButton] -- add your button handler code here..
        if (isForTrial)
        {
            if (quitButton->getButtonText() == "Quit")
            {
                JUCEApplication::getInstance()->systemRequestedQuit();
            }
            else if (quitButton->getButtonText() == "Continue")
            {
                if (auto* dialog = findParentComponentOfClass<ActivationDialog>())
                    dialog->closeButtonPressed();
            }
            else
            {
                jassertfalse; // unhandled quit button action
                if (auto* dialog = findParentComponentOfClass<ActivationDialog>())
                    dialog->closeButtonPressed();
            }
        }
        else if (isForRegistration)
        {
            setForRegistration (false);
        }
        else
        {
           #if EL_ALLOW_TRIAL_REGISTRATION
            setForRegistration (true);
           #else
            JUCEApplication::getInstance()->systemRequestedQuit();
           #endif
        }
        //[/UserButtonCode_quitButton]
    }

    //[UserbuttonClicked_Post]
    else if (buttonThatWasClicked == syncButton.get())
    {
        auto* const unlockRef = new UnlockOverlay (unlock,
            status, gui.getWorld(), UnlockOverlay::Check,
            status.getLicenseKey());
        unlockRef->setOpacity (0.72f);
        unlockRef->onFinished = std::bind (&ActivationComponent::handleActivationResult, this,
                                           std::placeholders::_1, std::placeholders::_2);
        addAndMakeVisible (unlock.get());
        resized();
    }
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void ActivationComponent::visibilityChanged()
{

}

void ActivationComponent::setForRegistration (bool setupRegistration)
{
    auto& status = gui.getWorld().getUnlockStatus();
    if (isForRegistration == setupRegistration)
        return;
    isForRegistration = setupRegistration;

    if (isForRegistration)
    {
        onlineActivateLink->setVisible(false);
        licenseKey->setVisible (false);
        instructionLabel2->setVisible (true);
        myLicenseLink->setVisible (false);
        getLicenseLink->setVisible (false);
        activateButton->setButtonText ("Register");
        quitButton->setButtonText ("Cancel");
        textBeforeReg = instructionLabel->getText();
        instructionLabel->setText (
            String("Activate your APPNAME trial by registering on kushview.net").replace ("APPNAME", Util::appName()),
            dontSendNotification);
        addAndMakeVisible (emailLabel);
        emailLabel.setText ("Email", dontSendNotification);
        emailLabel.setJustificationType (Justification::centredLeft);
        emailLabel.setFont (Font (13.f));
        addAndMakeVisible (email);

        addAndMakeVisible (userNameLabel);
        userNameLabel.setText ("Username", dontSendNotification);
        userNameLabel.setJustificationType (Justification::centredLeft);
        userNameLabel.setFont (Font (13.f));
        addAndMakeVisible (username);

        addAndMakeVisible (passwordLabel);
        passwordLabel.setText ("Password", dontSendNotification);
        passwordLabel.setJustificationType (Justification::centredLeft);
        passwordLabel.setFont (Font (13.f));
        addAndMakeVisible (password);

        email.grabKeyboardFocus();
    }
    else
    {
        instructionLabel->setText (textBeforeReg, dontSendNotification);
        licenseKey->setVisible (true);
        onlineActivateLink->setVisible (true);
        instructionLabel2->setVisible (false);
        myLicenseLink->setVisible (true);
        getLicenseLink->setVisible (true);
        activateButton->setButtonText ("Activate");
        quitButton->setButtonText ("Start Trial");
        licenseKey->grabKeyboardFocus();
        removeChildComponent (&emailLabel);
        removeChildComponent (&email);
        removeChildComponent (&userNameLabel);
        removeChildComponent (&username);
        removeChildComponent (&passwordLabel);
        removeChildComponent (&password);
    }

    resized();
    repaint();
}

void ActivationComponent::setForTrial (bool setupForTrial)
{
    auto& status = gui.getWorld().getUnlockStatus();
    if (! status.isTrial())
    {
        jassertfalse; // not a trial???
        return;
    }

    if (isForTrial == setupForTrial || setupForTrial == false)
        return;
    isForTrial = setupForTrial;
    licenseKey->setVisible (false);
    registerTrialLink->setVisible (false);
    addAndMakeVisible (progressBar);
    addAndMakeVisible (syncButton.get());
    progressBar.periodDays = status.getTrialPeriodDays();

    if (EL_IS_TRIAL_EXPIRED (status))
    {
        progress = 1.0;
        activateButton->setButtonText ("Purchase");
        quitButton->setButtonText ("Quit");
        instructionLabel->setText (
            String("Your trial license for APPNAME has expired. Use the button "
            "below to purchase a license.").replace("APPNAME", Util::appName()),
            dontSendNotification);
    }
    else
    {
        auto remaining = (double) status.getExpiryTime().toMilliseconds() - (double) Time::getCurrentTime().toMilliseconds();
        auto period = (double) RelativeTime::days(progressBar.periodDays).inMilliseconds();
        progress = (period - remaining) / period;
        progress = jlimit(0.0, 0.9999, progress);
        activateButton->setButtonText ("Upgrade");
        quitButton->setButtonText ("Continue");
        instructionLabel->setText (
            String("We hope you're enjoying APPNAME! Upgrade your trial license before expiration "
            "for a discounted price.").replace("APPNAME", Util::appName()),
            dontSendNotification);
    }

    resized();
}

void ActivationComponent::timerCallback()
{
    if (isForTrial || isForRegistration || grabbedFirstFocus)
        return stopTimer();

    if (auto* toplevel = getTopLevelComponent())
    {
        if (! toplevel->isOnDesktop())
            return;
        grabbedFirstFocus = true;
        licenseKey->grabKeyboardFocus();
        stopTimer();
    }
}

void ActivationComponent::handleActivationResult (const UnlockStatus::UnlockResult result, UnlockOverlay::Action)
{
    if (result.succeeded)
    {
        auto& _status = gui.getWorld().getUnlockStatus();
        if (EL_IS_TRIAL_EXPIRED(_status) || EL_IS_TRIAL_NOT_EXPIRED(_status))
        {
            isForTrial = false;
            setForTrial (true);
            resized();
        }
        else if (auto* dialog = findParentComponentOfClass<ActivationDialog>())
        {
            dialog->closeButtonPressed();
        }
        _status.refreshed();
    }
}

} // namespace Element
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="ActivationComponent" componentName=""
                 parentClasses="public Component, private Timer" constructorParams="GuiController&amp; g"
                 variableInitialisers="gui(g), progressBar(progress)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="480" initialHeight="346">
  <BACKGROUND backgroundColour="ff323e44"/>
  <LABEL name="AppNameLabel" id="98725c68017cae68" memberName="appNameLabel"
         virtualName="" explicitFocusOrder="7" pos="0Cc 37 320 48" edTextCol="ff000000"
         edBkgCol="0" labelText="ELEMENT" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="44.0"
         kerning="0.0" bold="0" italic="0" justification="36"/>
  <HYPERLINKBUTTON name="OnlineActivateLink" id="1b6f6eff8e1214e9" memberName="onlineActivateLink"
                   virtualName="" explicitFocusOrder="8" pos="0Cc 94 108 18" tooltip="EL_URL_HELP_ACTIVATION"
                   buttonText="Online Activation" connectedEdges="0" needsCallback="0"
                   radioGroupId="0" url="EL_URL_HELP_ACTIVATION"/>
  <LABEL name="InstructionLabel" id="47720f9959e590a" memberName="instructionLabel"
         virtualName="" explicitFocusOrder="9" pos="0Cc 124 352 39" edTextCol="ff000000"
         edBkgCol="0" labelText="APPNAME requires activation to run.  &#10;Please enter your FULL or TRIAL license key for APPNAME.  "
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="14.0" kerning="0.0" bold="0"
         italic="0" justification="36"/>
  <TEXTEDITOR name="LicenseKey" id="f335e055cddf091a" memberName="licenseKey"
              virtualName="" explicitFocusOrder="1" pos="0Cc 171 260 22" initialText=""
              multiline="0" retKeyStartsLine="0" readonly="0" scrollbars="1"
              caret="1" popupmenu="0"/>
  <TEXTBUTTON name="ActivateButton" id="38181d6f9efb97c5" memberName="activateButton"
              virtualName="" explicitFocusOrder="2" pos="-3Cr 208 90 24" posRelativeX="f335e055cddf091a"
              buttonText="Activate" connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="QuitButton" id="7b37f8d33c9651" memberName="quitButton"
              virtualName="" explicitFocusOrder="3" pos="3C 208 90 24" posRelativeX="f335e055cddf091a"
              buttonText="Start Trial" connectedEdges="0" needsCallback="1"
              radioGroupId="0"/>
  <HYPERLINKBUTTON name="MyLicensesLink" id="e6525e3d0946afd7" memberName="myLicenseLink"
                   virtualName="" explicitFocusOrder="4" pos="0Cc 249 96 18" tooltip="http://www.juce.com"
                   buttonText="My Licenses" connectedEdges="0" needsCallback="0"
                   radioGroupId="0" url="http://www.juce.com"/>
  <HYPERLINKBUTTON name="GetLicenseLink" id="3505ea719a627714" memberName="getLicenseLink"
                   virtualName="" explicitFocusOrder="5" pos="0Cc 272 94 18" tooltip="https://kushview.net/element/purchase"
                   buttonText="Get A License" connectedEdges="0" needsCallback="0"
                   radioGroupId="0" url="https://kushview.net/element/purchase"/>
  <HYPERLINKBUTTON name="RegisterTrialLink" id="c01a1273f8522474" memberName="registerTrialLink"
                   virtualName="" explicitFocusOrder="6" pos="0Cc 293 100 18" tooltip="https://kushview.net/element/trial"
                   buttonText="Register For Trial" connectedEdges="0" needsCallback="0"
                   radioGroupId="0" url="https://kushview.net/element/trial"/>
  <LABEL name="InstructionLabel" id="114f346dab5fa0b4" memberName="instructionLabel2"
         virtualName="" explicitFocusOrder="9" pos="0Cc 240 280 64" textCol="fff2f2f2"
         edTextCol="ff000000" edBkgCol="0" labelText="Your license key will be emailed to you upon sucessful registration. Fake credentials won't cut it."
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="14.0" kerning="0.0" bold="0"
         italic="1" justification="36" typefaceStyle="Italic"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]

