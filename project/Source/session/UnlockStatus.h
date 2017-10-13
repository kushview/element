
#pragma once

#include "ElementApp.h"
#define EL_LITE_VERSION_DEV

namespace Element {
    
class Globals;
class Settings;

class UnlockStatus :  public kv::EDDOnlineUnlockStatus
{
public:
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
       #ifdef EL_LITE_VERSION_DEV
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
       #if JUCE_DEBUG
        DBG("UNLOCKED: " << ((bool) isUnlocked() ? "yes" : "no"));
        DBG("LICENSE:  " << getLicenseKey());
        DBG("FULL:     " << ((bool) isFullVersion() ? "yes" : "no"));
        DBG("PRICE ID  " << (int) getProperty ("price_id"));
       #endif
    }
    
    inline void loadProps()
    {
        props = ValueTree (propsKey);
       #ifdef EL_LITE_VERSION_DEV
        const var priceId = getProperty(priceIdKey);
        const var two (2);
        if (two == priceId)
            props.setProperty (fullKey, 1, nullptr);
       #endif
    }
};

}
