
#pragma once

#include "ElementApp.h"

#if EL_USE_LOCAL_AUTH
  #define EL_PRO_PRODUCT_ID "19"
  #define EL_PRO_PRODUCT_ID_INT 19
  #define EL_PRO_PRICE_ID 1
  #define EL_PRO_MONTHLY_PRICE_ID 3
  #define EL_PRO_YEARLY_PRICE_ID 4
  #define EL_TRIAL_PRICE_ID 2
  
  #define EL_SOLO_PRODUCT_ID       "782"
  #define EL_SOLO_PRODUCT_ID_INT    782
  #define EL_SOLO_PRICE_ID          1
  #define EL_SOLO_MONTHLY_PRICE_ID  2
  #define EL_SOLO_YEARLY_PRICE_ID   3

  #define EL_LITE_PRICE_ID 999

  #define EL_BASE_URL "http://local.kushview.net"
  #define EL_AUTH_URL EL_BASE_URL "/edd-cp"
  #define EL_PUBKEY "3,753d95fa8511b392b09e5800043d41d1a7b2d330705f5714dcf2b31c8e22a7e9"
  #define EL_PRIVKEY "4e290ea703612261cb143aaaad7e2be03282bec968eea82b7d064226e66321ab,753d95fa8511b392b09e5800043d41d1a7b2d330705f5714dcf2b31c8e22a7e9"
  
  #define EL_LICENSE_SOLO                   "e7f6f0caa5cb90d38902ce141c7eeaa9"
  #define EL_LICENSE_SOLO_MONTHLY           "31812bb1002646ccec58f5961703fbaf"
  #define EL_LICENSE_SOLO_MONTHLY_EXPIRED   "b91672e4a6a11c6d8f6786d28ddf8dd3"

  #define EL_LICENSE_PRO                    "4d5728f3e654012acb9d0179ea85964c"
  #define EL_LICENSE_PRO_2                  "b5f4ac752e13c86ac244a100bab0c700"

  #define EL_LICENSE_PRO_MONTHLY            "aea1b1d8f1bdec3a58d7d20f55b85872"
  #define EL_LICENSE_PRO_MONTHLY_EXPIRED    "0a0b75419133f81ef5c1023046745119"
  #define EL_LICENSE_PRO_YEARLY             "713bcf0d2ff9221ed0f6a20e252daa40"
  #define EL_LICENSE_PRO_YEARLY_EXPIRED     "772d1ee713b558d82abf6f66411d3008"

  #define EL_LICENSE_TRIAL                  "3d71b8c414fa2ca5e8a0263658741c61"
  #define EL_LICENSE_TRIAL_2                "d7cc1a6dd8d4546f8ebbe3f9f5a2e779"
  #define EL_LICENSE_TRIAL_EXPIRED          "54c67bf48e84dfdb9a63bf7e1956e80b"

#else
 #define EL_PUBKEY_ENCODED 1

 #define EL_PRO_PRODUCT_ID "20"
 #define EL_PRO_PRODUCT_ID_INT 20
 #define EL_PRO_PRICE_ID 1
 #define EL_PRO_MONTHLY_PRICE_ID 3
 #define EL_PRO_YEARLY_PRICE_ID 4
 #define EL_TRIAL_PRICE_ID 2

 #define EL_SOLO_PRODUCT_ID       "10365"
 #define EL_SOLO_PRODUCT_ID_INT    10365
 #define EL_SOLO_PRICE_ID          1
 #define EL_SOLO_MONTHLY_PRICE_ID  2
 #define EL_SOLO_YEARLY_PRICE_ID   3
 
 #define EL_LITE_PRICE_ID 999

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

// we use macros to make it slightly harder for crackers

// true if this is a trial license which is expired
#define EL_IS_TRIAL_EXPIRED(status) ((status).isTrial() && (status).getExpiryTime() > Time() &&  (status).getExpiryTime() < Time::getCurrentTime())

// true if this is a trial license that is not expired
#define EL_IS_TRIAL_NOT_EXPIRED(status) ((status).isTrial() && (status).getExpiryTime() > Time() &&  (status).getExpiryTime() >= Time::getCurrentTime())

// false if not activated by any means at all
#define EL_IS_NOT_ACTIVATED(status) (!(status).isUnlocked() && !(status).isFullVersion())

// true if activated perpetual or subscription license
#define EL_IS_ACTIVATED(status) (((status).isUnlocked() && (status).isFullVersion()) \
|| (!(status).isTrial() && (status).isFullVersion() && !(status).isUnlocked() && (status).getExpiryTime() > Time() && (status).getExpiryTime() >= Time::getCurrentTime()))

// true if full activation or trial
#define EL_IS_ACTIVATED_OR_TRIAL(status) (EL_IS_TRIAL_NOT_EXPIRED((status)) || EL_IS_ACTIVATED((status)))

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

    UnlockResult registerTrial (const String& email, const String& username,
                                const String& password);

    /** Returns the number of days the license lasts for.
        This is expiration time minus creation time in days

        NOTE: this will need modified once we switch to a subscription
        based system and licenses can be renewed. The server should probably
        just return a "remaining days" property that we simply read here w/
        no math involved.
      */
    inline double getExpirationPeriodDays() const
    {
        if (! hasCreationTime())
            return 30.0;
        auto periodMillis = getExpiryTime().toMilliseconds() - getCreationTime().toMilliseconds();
        return RelativeTime::milliseconds(periodMillis).inDays();
    }
    
    /** Returns the time this license was created. If the creation date wasn't
        returned by the server this will return Time() so act accordingly */
    inline Time getCreationTime() const
    {
        return hasProperty ("created")
            ? Time (1000 * (int64) getProperty ("created"))
            : Time();
    }

    inline bool hasCreationTime() const     { return getCreationTime() > Time(); }
    inline var getPaymentID() const         { return getProperty ("payment_id", 0); }
    inline var getLicenseID() const         { return getProperty ("license_id", 0); }

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
        const char* productKey = proKey;
       #else
        #pragma error "Didn't get suitable product key"
       #endif

        if (getExpiryTime() > Time() && Time::getCurrentTime() < getExpiryTime())
            return !(bool)isUnlocked() && (props [trialKey] || props [productKey]);

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
    static const char* proKey;
    static const char* priceIdKey;
    static const char* trialKey;
    static const char* soloKey;
    static const char* downloadIdKey;
    
    Atomic<int> cancelled { 0 };
    Settings& settings;
    ValueTree props;
    UnlockResult result;

    void run() override;
    void timerCallback() override;
    void loadProps();
};

}
