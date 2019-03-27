#pragma once

#include "ElementApp.h"
#include "gui/widgets/Spinner.h"
#include "session/UnlockStatus.h"
#include "Globals.h"
#include "Settings.h"

namespace Element {


#if 1
struct UnlockOverlay : public Component,
                       private Thread,
                       private Timer
{
    enum Action {
        Activate,
        Deactivate,
        Check,
        Register
    };

    typedef std::function<void(UnlockStatus::UnlockResult result, UnlockOverlay::Action actionPerformed)> FinishedFunction;
    FinishedFunction onFinished;

    UnlockOverlay (std::unique_ptr<Component>& ownerComponent,
                   UnlockStatus& unlockStatusToUse,
                   Globals& globalObjects,
                   Action actionToPerform,
                   const String& licenseKeyToUse,
                   const String& emailToRegister = String(),
                   const String& usernameToRegister = String(),
                   const String& passwordToRegister = String(),
                   bool deactivateOtherMachines = false)
        : Thread (String()),
          owner (ownerComponent),
          action (actionToPerform),
          status (unlockStatusToUse),
          world (globalObjects),
          deactivateOthers (deactivateOtherMachines)
    {
        owner.reset (this);
        result.succeeded = false;
        email       = emailToRegister.trim();
        password    = passwordToRegister.trim();
        username    = usernameToRegister.trim();
        license     = licenseKeyToUse;
        addAndMakeVisible (spinner);
        startThread (4);
    }

    ~UnlockOverlay()
    {
        stopThread (10000);
    }

    void setShowText (bool showIt)
    {
        showText = showIt;
        repaint();
    }

    void setOpacity (float newOpacity)
    {
        opacity = jlimit (0.0f, 1.f, newOpacity);
        repaint();
    }

    void paint (Graphics& g) override
    {
        g.fillAll (Colours::transparentBlack.withAlpha (opacity));

        if (showText)
        {
            g.setColour (LookAndFeel_KV1::textColor);
            g.setFont (18.0f);

            g.drawFittedText (TRANS("Contacting XYZ...").replace ("XYZ", status.getWebsiteName()),
                            getLocalBounds().reduced (20, 0).removeFromTop (proportionOfHeight (0.6f)),
                            Justification::centred, 5);
        }
    }

    void resized() override
    {
        const int spinnerSize = 80;
        spinner.setBounds ((getWidth() - spinnerSize) / 2, proportionOfHeight (0.5f), spinnerSize, spinnerSize);
    }

    void clearLicense()
    {
        status.saveState (String());
        status.loadAll();
    }

    void run() override
    {
        if (! areMajorWebsitesAvailable() || ! canConnectToWebsite (URL (EL_BASE_URL), 10000))
        {
            connectionError.set (1);
            startTimer(200);
            return;
        }

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
                result = status.deactivateLicense (license);
            } break;

            case Activate:
            {
                if (deactivateOthers)
                    params.set ("deactivate_others", "1");
                result = status.activateLicense (license, {}, {}, params);
            } break;

            case Check:
            {
                result = status.checkLicense (license, params);
            } break;

            case Register:
            {
                result = status.registerTrial (email, username, password);
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
            case Register:      title = "Activate Trial"; break;
        }

        title << (failed ? " Failed" : " Complete");
        return TRANS (title);
    }

    void timerCallback() override
    {
        spinner.setVisible (false);
        stopTimer();

        if (connectionError.get() > 0)
        {
            AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                              getActionTitle (true),
                                              "Could not connect to the network.");
        }
        else if (result.errorMessage.isNotEmpty())
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
        const auto _result    = result;
        auto& _world          = world;
        const auto _action    = action;
        UnlockStatus& _status = status;
        auto _onFinished      = onFinished;
        bool _connectionError = connectionError.get() > 0;
        owner.reset();

        if (_connectionError)
            return;
        
        if (_result.succeeded && (_action == Activate || _action == Check))
        {
            _status.save();
            _status.loadAll();
        }
        else if (_result.succeeded && _action == Deactivate)
        {
            _status.saveState (String());
            _status.loadAll();
        }
        else if (! _result.succeeded && (_action == Activate || _action == Check) && _status.isExpiring())
        {
            _status.save();
            _status.loadAll();
        }

        _world.getSettings().saveIfNeeded();
        
        if (_onFinished)
            _onFinished (_result, _action);
        
        _status.refreshed();
    }

    Atomic<int> connectionError { 0 };
    std::unique_ptr<Component>& owner;
    float opacity { 0.5f };
    bool showText = true;
    const Action action;
    UnlockStatus& status;
    Globals& world;
    bool deactivateOthers = false;
    Spinner spinner;
    OnlineUnlockStatus::UnlockResult result;
    String email, username, password, license;

    JUCE_LEAK_DETECTOR (UnlockOverlay)
};
#endif

}
