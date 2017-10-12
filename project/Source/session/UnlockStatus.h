
#pragma once

#include "ElementApp.h"

namespace Element {
    
class Globals;
class Settings;

class UnlockStatus :  public kv::EDDOnlineUnlockStatus
{
public:
    UnlockStatus (Globals&);
    ~UnlockStatus() { }
    
    void loadAll()
    {
        load();
        loadProps();
        dump();
    }
    
    String getProductID() override;
    bool doesProductIDMatch (const String& returnedIDFromServer) override;
    RSAKey getPublicKey() override;
    void saveState (const String&) override;
    String getState() override;
    String getWebsiteName() override;
    URL getServerAuthenticationURL() override;
    StringArray getLocalMachineIDs() override;

    inline var isFullVersion() const
    {
       #ifdef EL_LITE_VERSION_DEV
        if (! (bool) isUnlocked())
            return var (false);
        return props["f"];
       #else
        return isUnlocked();
       #endif
    }
    
    inline void dump()
    {
       #if JUCE_DEBUG
        DBG("UNLOCKED: " << ((bool) isUnlocked() ? "yes" : "no"));
        DBG("LICENSE:  " << getLicenseKey());
        DBG("FULL:     " << ((bool) isFullVersion() ? "yes" : "no"));
       #endif
    }
    
private:
    Settings& settings;
    ValueTree props;
    
    inline void loadProps()
    {
        props = ValueTree ("props");
       #ifdef EL_LITE_VERSION_DEV
        const String idsStr = getProperty("price_ids").toString();
        StringArray pids;
        pids.addTokens (idsStr, ",", "'");
        for (const auto& pid : pids)
            if (2 == atoi (pid.toRawUTF8()))
                props.setProperty ("f", true, nullptr);
       #endif
    }
};

}
