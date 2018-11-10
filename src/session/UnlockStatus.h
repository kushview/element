
#pragma once

#include "ElementApp.h"

#ifndef EL_DEBUG_LICENSE
 #define EL_DEBUG_LICENSE 1
#endif

#ifndef EL_DISABLE_UNLOCKING
 #define EL_DISABLE_UNLOCKING 0
#endif

#ifndef EL_USE_LOCAL_AUTH
 #define EL_USE_LOCAL_AUTH 0
#endif

namespace Element {
    
class Globals;
class Settings;

class UnlockStatus :  public kv::EDDOnlineUnlockStatus,
                      private Thread, private Timer
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

    inline var isExpiring() const
    {
        var result (0);
        if (getExpiryTime() > Time())
        {
            var yes (1);
            result.swapWith (yes);
        }

        return result;
    }

    inline var isTrial() const { return props [trialKey]; }

    inline var isFullVersion() const
    {
       #if EL_DISABLE_UNLOCKING
        return var (true);
       #endif

        if (getExpiryTime() > Time() && Time::getCurrentTime() < getExpiryTime())
            return !isUnlocked() && (props [trialKey] || props [fullKey]);

        if (! (bool) isUnlocked())
            return isUnlocked();
        return props [fullKey];
    }
    
    void launchProUpgradeUrl();
    
    String getProductID() override;
    bool doesProductIDMatch (const String& returnedIDFromServer) override;
    RSAKey getPublicKey() override;
    void saveState (const String&) override;
    String getState() override;
    String getWebsiteName() override;
    URL getServerAuthenticationURL() override;
    StringArray getLocalMachineIDs() override;

    inline void checkLicenseInBackground() {
        if (isThreadRunning())
            return;
        startThread (4);
    }

    Signal refreshed;

private:
    friend class Thread;
    static const char* propsKey;
    static const char* fullKey;
    static const char* priceIdKey;
    static const char* trialKey;
    
    Settings& settings;
    ValueTree props;
    UnlockResult result;

    void run() override;
    void timerCallback() override;
    void loadProps();
    
    inline void dump()
    {
       #if EL_DEBUG_LICENSE
        DBG("[EL] isUnlocked(): " << ((bool) isUnlocked() ? "yes" : "no"));
        DBG("[EL] isTrial(): " << ((bool)isTrial() ? "yes" : "no"));
        DBG("[EL] getLicenseKey(): " << getLicenseKey());
        DBG("[EL] isExpiring(): " << ((bool) isExpiring() ? "yes" : "no"));
        DBG("[EL] isFullVersion(): " << ((bool) isFullVersion() ? "yes" : "no"));
        DBG("[EL] getProperty('price_id')  " << (int) getProperty ("price_id"));
        DBG("[EL] getExpiryTime(): " << getExpiryTime().toString (true, true));
        DBG(props.toXmlString());
       #endif
    }

};

}
