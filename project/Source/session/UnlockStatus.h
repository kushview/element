
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
    
        inline void dump()
        {
            
            DBG (edd.toXmlString());
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
