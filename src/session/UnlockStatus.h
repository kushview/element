
#pragma once

#include "ElementApp.h"

#ifndef EL_DEBUG_LICENSE
 #define EL_DEBUG_LICENSE 1
#endif

#ifndef EL_LITE_VERSION_DEV
 #define EL_LITE_VERSION_DEV 1
#endif

#ifndef EL_DISABLE_UNLOCKING
 #define EL_DISABLE_UNLOCKING 0
#endif

namespace Element {
    
class Globals;
class Settings;

class UnlockStatus :  public kv::EDDOnlineUnlockStatus
{
public:
    struct LockableObject 
    {
        virtual ~LockableObject() noexcept { }
        virtual void setLocked (const var& locked) =0;
        virtual void showLockedAlert();
    };

    UnlockStatus (Globals&);
    ~UnlockStatus() { }

    inline void loadAll()
    {
        load();
        loadProps();
    }

    inline var isTrial() const
    {
        var result (0);
        if (getExpiryTime() > Time() && Time::getCurrentTime() <= getExpiryTime())
        {
            var yes (1);
            result.swapWith (yes);
        }

        return result;
    }

    inline var hasTrialExpired() const
    {
        var result (0);

        if (getExpiryTime() > Time() && Time::getCurrentTime() > getExpiryTime())
        {
            var yes (1);
            result.swapWith (yes);
        }

        return result;
    }

    inline var isFullVersion() const
    {
       #if EL_DISABLE_UNLOCKING
        return var (true);
        
       #elif EL_LITE_VERSION_DEV

        if (! (bool) isUnlocked())
            return var (0);
        return props [fullKey];
        
       #else
        return isUnlocked();
       #endif
    }
    
    void launchProUpgradeUrl();
    
    UnlockStatus::UnlockResult activateTrial (const String& email);

    String getProductID() override;
    bool doesProductIDMatch (const String& returnedIDFromServer) override;
    RSAKey getPublicKey() override;
    void saveState (const String&) override;
    String getState() override;
    String getWebsiteName() override;
    URL getServerAuthenticationURL() override;
    StringArray getLocalMachineIDs() override;

    inline void dump()
    {
       #if EL_DEBUG_LICENSE
        DBG("[EL] isUnlocked(): " << ((bool) isUnlocked() ? "yes" : "no"));
        DBG("[EL] getLicenseKey(): " << getLicenseKey());
        DBG("[EL] isTrial(): " << ((bool) isTrial() ? "yes" : "no"));
        DBG("[EL] isFullVersion(): " << ((bool) isFullVersion() ? "yes" : "no"));
        DBG("[EL] getProperty('price_id')  " << (int) getProperty ("price_id"));
        DBG("[EL] getExpiryTime(): " << getExpiryTime().toString (true, true));
        DBG("[EL] hasTrialExpired(): " << ((bool) hasTrialExpired() ? "yes" : "no"));
       #endif
    }

private:
    static const char* propsKey;
    static const char* fullKey;
    static const char* priceIdKey;
    
    Settings& settings;
    ValueTree props;
    
    inline void loadProps()
    {
        props = ValueTree (propsKey);
       #if EL_LITE_VERSION_DEV
        const var priceId = getProperty (priceIdKey);
        const var two (2); const var zero (0);
        if (two == priceId || zero == priceId)
            props.setProperty (fullKey, 1, nullptr);
       #endif
    }
};

}
