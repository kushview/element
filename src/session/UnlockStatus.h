
#pragma once

#include "ElementApp.h"

#if EL_USE_LOCAL_AUTH
 // Element Pro
 #define EL_PRODUCT_ID "19"
 #define EL_PRO_PRICE_ID 1
 #define EL_TRIAL_PRICE_ID 2
 #define EL_SOLO_PRICE_ID 3

 #define EL_BASE_URL "http://local.kushview.net"
  #define EL_AUTH_URL EL_BASE_URL "/edd-cp"
  #define EL_PUBKEY "3,753d95fa8511b392b09e5800043d41d1a7b2d330705f5714dcf2b31c8e22a7e9"
  #define EL_PRIVKEY "4e290ea703612261cb143aaaad7e2be03282bec968eea82b7d064226e66321ab,753d95fa8511b392b09e5800043d41d1a7b2d330705f5714dcf2b31c8e22a7e9"
  
  #define EL_LICENSE_SOLO   "b5f4ac752e13c86ac244a100bab0c700"
  #define EL_LICENSE_PRO    "1f673dfaa0aea1b913efb01d4243ef53"
  #define EL_LICENSE_TRIAL  "3d71b8c414fa2ca5e8a0263658741c61"
  #define EL_LICENSE_TRIAL_EXPIRED  "54c67bf48e84dfdb9a63bf7e1956e80b"

#elif EL_USE_CI_AUTH
 #define EL_PRODUCT_ID "12"
 #define EL_TRIAL_PRICE_ID 2
 #define EL_PRO_PRICE_ID 1
 #define EL_BASE_URL "https://ci.kushview.net"
 #define EL_AUTH_URL EL_BASE_URL "/edd-cp"
 #define EL_PUBKEY "5,7bc48fe0cef16975604686123c4b8c9f597b8d62b839e14f6300e632f993d613c406c068ecccd912c845ab314574ae727d55ef1ce8257e6d6dfd239a1cf5831753632a8eb9615d1033264e132edfcf537bc1e643288f45138e364fb2e2afe91c43ceaf929209d3d26428f6f276242b8505e63a923702f3990000fa043a324473"

#else
 #define EL_PUBKEY_ENCODED 1
 #define EL_PRO_PRICE_ID 1
 #define EL_TRIAL_PRICE_ID 2
 #define EL_SOLO_PRICE_ID 3

 #define EL_PRODUCT_ID "20"
 #define EL_BASE_URL "https://kushview.net"
 #define EL_AUTH_URL EL_BASE_URL "/products/authorize"
#endif

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
        
       #if defined (EL_SOLO)
        const char* productKey = soloKey;
       #elif defined (EL_FREE)
        const char* productKey = liteKey;
       #elif defined (EL_PRO)
        const char* productKey = fullKey;
       #else
        #pragma error "Didn't get suitable product key"
       #endif

        if (getExpiryTime() > Time() && Time::getCurrentTime() < getExpiryTime())
            return !isUnlocked() && (props [trialKey] || props [productKey]);

        if (! (bool) isUnlocked())
            return isUnlocked();
        return props [productKey];
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
