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

#pragma once

//[Headers]     -- You can add your own extra header files here --
#include "ElementApp.h"
#include "controllers/GuiController.h"
#include "gui/UnlockOverlay.h"
#include "session/CommandManager.h"
#include "session/UnlockStatus.h"
#include "Globals.h"
#include "Utils.h"

namespace Element {

class ContentComponent;

struct TrialDaysProgressBar : public ProgressBar
{
    explicit TrialDaysProgressBar (double& progress)
        : ProgressBar (progress) { }
    double periodDays = 14.0;
};

//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class ActivationComponent  : public Component,
                             private Timer,
                             public Button::Listener
{
public:
    //==============================================================================
    ActivationComponent (GuiController& g);
    ~ActivationComponent();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void setForTrial (bool setupForTrial, bool refreshButton = false);
    void setForRegistration (bool setupRegistration);
    void visibilityChanged() override;
    void timerCallback() override;
    void setBackgroundColour (const Colour& color) { backgroundColour = color; repaint(); }
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void buttonClicked (Button* buttonThatWasClicked) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    Colour backgroundColour;
    GuiController& gui;
    std::unique_ptr<Component> unlock;
    double progress = 0.0;
    TrialDaysProgressBar progressBar;
    bool isForTrial = false;
    bool isForRegistration = false;
    String textBeforeReg;
    Label emailLabel { "Email"};
    TextEditor email;
    Label userNameLabel { "Username" };
    TextEditor username;
    Label passwordLabel { "Password" };
    TextEditor password;
    bool grabbedFirstFocus = false;

    void handleActivationResult (const UnlockStatus::UnlockResult result, UnlockOverlay::Action);
    //[/UserVariables]

    //==============================================================================
    std::unique_ptr<Label> appNameLabel;
    std::unique_ptr<HyperlinkButton> onlineActivateLink;
    std::unique_ptr<Label> instructionLabel;
    std::unique_ptr<TextEditor> licenseKey;
    std::unique_ptr<TextButton> activateButton;
    std::unique_ptr<TextButton> quitButton;
    std::unique_ptr<HyperlinkButton> myLicenseLink;
    std::unique_ptr<HyperlinkButton> getLicenseLink;
    std::unique_ptr<HyperlinkButton> registerTrialLink;
    std::unique_ptr<Label> instructionLabel2;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ActivationComponent)
};

//[EndFile] You can add extra defines here...

class ActivationDialog : public DialogWindow
{
public:
    ActivationDialog (GuiController& g, std::unique_ptr<Component>& o)
        : DialogWindow (Util::appName ("Activate"),
                        g.getLookAndFeel().findColour (DocumentWindow::backgroundColourId),
                        true, false),
          owner (o), gui (g)
    {
        setEnabled (true);
        owner.reset (this);
        setUsingNativeTitleBar (true);

        auto* const activation = new ActivationComponent (gui);
        auto& status = g.getWorld().getUnlockStatus();
        if ((EL_IS_TRIAL_EXPIRED(status)) || (EL_IS_TRIAL_NOT_EXPIRED(status)))
            activation->setForTrial (true);
        setContentOwned (activation, true);
        centreAroundComponent ((Component*) gui.getContentComponent(), getWidth(), getHeight());
        setAlwaysOnTop (true);
        addToDesktop();
        setVisible (true);
    }

    void closeButtonPressed() override
    {
        owner.reset();
    }

private:
    std::unique_ptr<Component>& owner;
    GuiController& gui;
};

} // namespace element
//[/EndFile]

