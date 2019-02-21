
#include "session/UnlockStatus.h"
#include "Globals.h"
#include "Settings.h"

#define EL_LICENSE_SETTINGS_KEY "L"



namespace Element {

    #include "./PublicKey.h"

    void UnlockStatus::LockableObject::showLockedAlert()
    {
        Alert::showProductLockedAlert();
    }

    const char* UnlockStatus::propsKey      = "props";
    const char* UnlockStatus::fullKey       = "f";
    const char* UnlockStatus::trialKey      = "t";
    const char* UnlockStatus::soloKey       = "s";
    const char* UnlockStatus::priceIdKey    = "price_id";
    
    UnlockStatus::UnlockStatus (Globals& g) : Thread("elt"), settings (g.getSettings()) { }
    String UnlockStatus::getProductID() { return EL_PRODUCT_ID; }

    bool UnlockStatus::doesProductIDMatch (const String& returnedIDFromServer)
    {
        const StringArray pids { getProductID() };
        return pids.contains (returnedIDFromServer);
    }
    
    RSAKey UnlockStatus::getPublicKey()
    {
       #if EL_PUBKEY_ENCODED
        return RSAKey (Element::getPublicKey());
       #elif defined (EL_PUBKEY)
        return RSAKey (EL_PUBKEY);
       #else
        jassertfalse; // you need a public key for unlocking features
       #endif
    }
    
    void UnlockStatus::saveState (const String& data)
    {
        if (auto* const props = settings.getUserSettings())
            props->setValue (EL_LICENSE_SETTINGS_KEY, data);
    }
    
    static URL elGetProUpgradeUrl()
    {
        return URL (EL_BASE_URL "/products/element?pro_upgrade=1");
    }
    
    void UnlockStatus::launchProUpgradeUrl()
    {
        elGetProUpgradeUrl().launchInDefaultBrowser();
    }
    
    String UnlockStatus::getState()
    {
        if (auto* const props = settings.getUserSettings())
        {
            const String value =  props->getValue (EL_LICENSE_SETTINGS_KEY).toStdString();
            eddRestoreState (value);
            return value;
        }
        return String();
    }
    
    String UnlockStatus::getWebsiteName() {
        return "Kushview";
    }
    
    URL UnlockStatus::getServerAuthenticationURL()
    {
        const URL authurl (EL_AUTH_URL);
        return authurl;
    }
    
    StringArray UnlockStatus::getLocalMachineIDs()
    {
        auto ids (OnlineUnlockStatus::getLocalMachineIDs());
        return ids;
    }

    void UnlockStatus::run()
    {
        const auto key (getLicenseKey());
        if (key.isEmpty())
            return;
        if (!areMajorWebsitesAvailable() || !canConnectToWebsite (URL (EL_BASE_URL), 10000)) {
            DBG("[EL] cannot connect to server to check license: " << EL_AUTH_URL);
            return;
        }
        
        if (cancelled.get() == 0)
        {
            result = checkLicense (key);
            startTimer (200);
        }
    }

    void UnlockStatus::timerCallback()
    {
        if (result.errorMessage.isNotEmpty())
        {
            DBG("[EL] check license error: " << result.errorMessage);
        }
        
        if (result.informativeMessage.isNotEmpty())
        {
            DBG("[EL] check license info: " << result.informativeMessage);;
        }
        
        if (result.urlToLaunch.isNotEmpty())
        {
            URL url (result.urlToLaunch);
            DBG("[EL] check url: " << url.toString (true));
        }

        save();
        loadAll();
        refreshed();
        stopTimer();
    }

    void UnlockStatus::loadProps()
    {
       #if defined (EL_FREE)
        props.removeProperty (fullKey, nullptr);
        props.removeProperty (trialKey, nullptr);
        props.removeProperty (soloKey, nullptr);
        return;
       #endif

        props = ValueTree (propsKey);
        const var full (EL_PRO_PRICE_ID); 
        const var trial (EL_TRIAL_PRICE_ID);
        const var solo (EL_SOLO_PRICE_ID);
        const var zero (0);

       #if defined (EL_PRO)
        if (full.equals (getProperty (priceIdKey)) || zero.equals (getProperty (priceIdKey)))
        {
            props.setProperty (fullKey, 1, nullptr);
            props.removeProperty (trialKey, nullptr);
            props.removeProperty (soloKey, nullptr);
        }
        else if (trial.equals (getProperty (priceIdKey)))
        {
            props.removeProperty (fullKey, nullptr);
            props.setProperty (trialKey, 1, nullptr);
            props.removeProperty (soloKey, nullptr);
        }
        else
        {
            props.removeProperty (fullKey, nullptr);
            props.removeProperty (trialKey, nullptr);
            props.removeProperty (soloKey, nullptr);
        }

       #elif defined (EL_SOLO)
        if (full.equals (getProperty (priceIdKey)) || zero.equals (getProperty (priceIdKey)))
        {
            props.setProperty (fullKey, 1, nullptr);
            props.removeProperty (trialKey, nullptr);
            props.setProperty (soloKey, 1, nullptr);
        }
        else if (trial.equals (getProperty (priceIdKey)))
        {
            props.removeProperty (fullKey, nullptr);
            props.setProperty (trialKey, 1, nullptr);
            props.removeProperty (soloKey, nullptr);
        }
        else if (solo.equals (getProperty (priceIdKey)))
        {
            props.removeProperty (fullKey, nullptr);
            props.removeProperty (trialKey, nullptr);
            props.setProperty (soloKey, 1, nullptr);
        }
        else
        {
            props.removeProperty (fullKey, nullptr);
            props.removeProperty (trialKey, nullptr);
            props.removeProperty (soloKey, nullptr);
        }
       #else
        #pragma error "Cannot deduce which product to authorize.  Fix logic!"
       #endif
    }

    void UnlockStatus::dump()
    {
       #if EL_DEBUG_LICENSE
        DBG("[EL] isUnlocked(): " << ((bool) isUnlocked() ? "yes" : "no"));
        DBG("[EL] isTrial(): " << ((bool)isTrial() ? "yes" : "no"));
        DBG("[EL] getLicenseKey(): " << getLicenseKey());
        DBG("[EL] isExpiring(): " << ((bool) isExpiring() ? "yes" : "no"));
        DBG("[EL] isFullVersion(): " << ((bool) isFullVersion() ? "yes" : "no"));
        DBG("[EL] getProperty('price_id')  " << (int) getProperty ("price_id"));
        DBG("[EL] getExpiryTime(): " << getExpiryTime().toString (true, true));
        DBG("[EL] full key: " << (int)props[fullKey]);
        DBG("[EL] trial key: " << (int)props[trialKey]);
        DBG("[EL] solo key: " << (int)props[soloKey]);
        DBG(props.toXmlString());
       #endif
    }
}
