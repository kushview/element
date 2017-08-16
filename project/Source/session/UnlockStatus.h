
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
        String getProductID() override;
        bool doesProductIDMatch (const String& returnedIDFromServer) override;
        RSAKey getPublicKey() override;
        void saveState (const String&) override;
        String getState() override;
        String getWebsiteName() override;
        URL getServerAuthenticationURL() override;
        StringArray getLocalMachineIDs() override;
    
        inline bool isFullVersion() const
        {
            if (! (bool) isUnlocked())
                return false;
            
            StringArray ids; Array<int> pids;
            ids.addTokens (edd["ownedPriceIDs"].toString(), ",", String());
            for (const auto& i : ids)
            {
                const int ii = atoi(i.trim().toRawUTF8());
                if (ii > 1) pids.add(ii);
            }
            
            return pids.contains (2);
        }
        
        inline void dump()
        {
           #if JUCE_DEBUG
            DBG("UNLOCKED: " << ((bool) isUnlocked() ? "yes" : "no"));
            DBG("DL ID:    " << edd["downloadID"].toString());
            DBG("PIDS:     " << edd["ownedPriceIDs"].toString());
            DBG("FULL:     " << (isFullVersion() ? "yes" : "no"));
            
            DBG("EDD XML:");
            DBG (edd.toXmlString());
           #endif
        }
        
    protected:
        inline bool useLicenseKey() const override { return false; }
        
        inline StringPairArray getQueryParams() override
        {
            StringPairArray params;
            params.set ("mach", getLocalMachineIDs()[0]);
            return params;
        }
        
    private:
        Settings& settings;
    };
}
