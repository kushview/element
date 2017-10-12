
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
            this->load();
            props = ValueTree();
            const String idsStr = getProperty("price_ids").toString();
            StringArray pids;
            pids.addTokens (idsStr, ",", "'");
            for (const auto& pid : pids)
                if (2 == atoi (pid.toRawUTF8()))
                    props.setProperty ("f", true, nullptr);
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
            if (! (bool) isUnlocked())
                return var (false);
            return props["f"];
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
    };
}
