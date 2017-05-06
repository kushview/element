
#pragma once

#include "ElementApp.h"

namespace Element {
    class Globals;
    class Settings;
    
    class UnlockStatus :  public OnlineUnlockStatus
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
        String readReplyFromWebserver (const String& email, const String& password) override;
        StringArray getLocalMachineIDs() override;

    private:
        Settings& settings;
    };
}
