
#pragma once

#include "ElementApp.h"

namespace Element {
    class UnlockStatus :  public OnlineUnlockStatus
    {
    public:
        String getProductID() override;
        bool doesProductIDMatch (const String& returnedIDFromServer) override;
        RSAKey getPublicKey() override;
        void saveState (const String&) override;
        String getState() override;
        String getWebsiteName() override;
        URL getServerAuthenticationURL() override;
        String readReplyFromWebserver (const String& email, const String& password) override;
        StringArray getLocalMachineIDs() override;
    };
}
