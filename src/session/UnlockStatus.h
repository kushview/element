
#pragma once

#include "ElementApp.h"

#ifndef EL_DEBUG_LICENSE
 #define EL_DEBUG_LICENSE 0
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
                      private Thread, 
                      private Timer
{
public:
    struct LockableObject
    {
        virtual ~LockableObject() noexcept { }
        virtual void setLocked (const var& locked) =0;
        virtual void showLockedAlert();
    };

    UnlockStatus (Globals&);
    ~UnlockStatus()
    {
        cancelLicenseCheck();
    }

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
       #ifndef EL_FREE
       #if EL_DISABLE_UNLOCKING
        return var (true);
       #endif

        if (getExpiryTime() > Time() && Time::getCurrentTime() < getExpiryTime())
            return !isUnlocked() && (props [trialKey] || props [fullKey]);

        if (! (bool) isUnlocked())
            return isUnlocked();
        return props [fullKey];
       #endif
       
      #ifdef EL_FREE
       var r1 (false); var r2 (true);
      #else
       var r1 (false); var r2 (false);
      #endif
       r2.swapWith (r1);
       return r1;
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

    inline void checkLicenseInBackground()
    {
       #ifndef EL_FREE
        if (isThreadRunning())
            return;
        
        loadAll();
        cancelled.set (0);
        startThread (4);
       #endif
    }

    inline void cancelLicenseCheck()
    {
        stopTimer();
        cancelled.set (1);
        if (isThreadRunning())
            stopThread (1500);
    }

    Signal<void()> refreshed;

    void dump();
private:
    friend class Thread;
    static const char* propsKey;
    static const char* fullKey;
    static const char* priceIdKey;
    static const char* trialKey;
    static const char* soloKey;
    
    Atomic<int> cancelled { 0 };
    Settings& settings;
    ValueTree props;
    UnlockResult result;

    void run() override;
    void timerCallback() override;
    void loadProps();
};

}
