
#include "session/UnlockStatus.h"
#include "Globals.h"
#include "Settings.h"

#define EL_LICENSE_SETTINGS_KEY "L"

#if EL_USE_LOCAL_AUTH
 // Element Pro
 #define EL_PRODUCT_ID "19"
 #define EL_TRIAL_PRICE_ID 2
 #define EL_PRO_PRICE_ID 1
 
 #define EL_BASE_URL "http://local.kushview.net"
 #define EL_AUTH_URL EL_BASE_URL "/edd-cp"
 #define EL_PUBKEY "3,753d95fa8511b392b09e5800043d41d1a7b2d330705f5714dcf2b31c8e22a7e9"
 #define EL_PRIVKEY "4e290ea703612261cb143aaaad7e2be03282bec968eea82b7d064226e66321ab,753d95fa8511b392b09e5800043d41d1a7b2d330705f5714dcf2b31c8e22a7e9"
 #define EL_LICENSE_1 "2de310c8ef18a6f6da40270399cc13f6" // Trial
 #define EL_LICENSE_2 ""
 #define EL_LICENSE_3 ""
 #define EL_LICENSE_4 ""

#elif EL_USE_CI_AUTH
 #define EL_PRODUCT_ID "12"
 #define EL_TRIAL_PRICE_ID 2
 #define EL_PRO_PRICE_ID 1
 #define EL_BASE_URL "https://ci.kushview.net"
 #define EL_AUTH_URL EL_BASE_URL "/edd-cp"
 #define EL_PUBKEY "5,7bc48fe0cef16975604686123c4b8c9f597b8d62b839e14f6300e632f993d613c406c068ecccd912c845ab314574ae727d55ef1ce8257e6d6dfd239a1cf5831753632a8eb9615d1033264e132edfcf537bc1e643288f45138e364fb2e2afe91c43ceaf929209d3d26428f6f276242b8505e63a923702f3990000fa043a324473"

#else
 #define EL_PUBKEY_ENCODED 1
 #define EL_TRIAL_PRICE_ID 2
 #define EL_PRO_PRICE_ID 1
 #define EL_PRODUCT_ID "20"
 #define EL_BASE_URL "https://kushview.net"
 #define EL_AUTH_URL EL_BASE_URL "/products/authorize"
#endif

namespace Element {

    #include "./PublicKey.h"

    void UnlockStatus::LockableObject::showLockedAlert()
    {
        Alert::showProductLockedAlert();
    }

    const char* UnlockStatus::propsKey = "props";
    const char* UnlockStatus::fullKey = "f";
    const char* UnlockStatus::trialKey = "t";
    const char* UnlockStatus::priceIdKey = "price_id";
    
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
    
    URL UnlockStatus::getServerAuthenticationURL() {
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
       #ifndef EL_FREE
        props = ValueTree (propsKey);
        const var full (EL_PRO_PRICE_ID); 
        const var trial (EL_TRIAL_PRICE_ID); 
        const var zero (0);

        if (full.equals (getProperty (priceIdKey)) || zero.equals (getProperty (priceIdKey)))
        {
            props.setProperty (fullKey, 1, nullptr);
            props.removeProperty (trialKey, nullptr);
        }
        else if (trial.equals (getProperty (priceIdKey)))
        {
            props.removeProperty (fullKey, nullptr);
            props.setProperty (trialKey, 1, nullptr);
        }
        else
        {
            props.removeProperty (fullKey, nullptr);
            props.removeProperty (trialKey, nullptr);
        }

       #else
        props.removeProperty (fullKey, nullptr);
        props.removeProperty (trialKey, nullptr);
       #endif
    }
}
