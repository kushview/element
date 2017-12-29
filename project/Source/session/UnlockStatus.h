
#pragma once

#include "ElementApp.h"

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
        dump();
    }

    inline var isFullVersion() const
    {
       #if EL_DISABLE_UNLOCKING
        return var(true);
        
       #elif EL_LITE_VERSION_DEV
        if (! (bool) isUnlocked())
            return var (0);
        return props [fullKey];
        
       #else
        return isUnlocked();
       #endif
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

private:
    static const char* propsKey;
    static const char* fullKey;
    static const char* priceIdKey;
    
    Settings& settings;
    ValueTree props;
    
    inline void dump()
    {
       #if EL_DEBUG_LICENSE
        DBG("[EL] unlocked: " << ((bool) isUnlocked() ? "yes" : "no"));
        DBG("[EL] license:  " << getLicenseKey());
        DBG("[EL] full:     " << ((bool) isFullVersion() ? "yes" : "no"));
        DBG("[EL] price id  " << (int) getProperty ("price_id"));
       #endif
    }
    
    inline void loadProps()
    {
        props = ValueTree (propsKey);
       #if EL_LITE_VERSION_DEV
        const var priceId = getProperty (priceIdKey);
        const var two (2); const var zero(0);
        if (two == priceId || zero == priceId)
            props.setProperty (fullKey, 1, nullptr);
       #endif
    }
};

}
