
#include "session/UnlockStatus.h"

#define EL_PRODUCT_ID "Element"

namespace Element {
    String UnlockStatus::getProductID()         { return "Element"; }
    
    bool UnlockStatus::doesProductIDMatch (const String& returnedIDFromServer)
    {
        return false;
    }
    
    RSAKey UnlockStatus::getPublicKey()
    {
        return RSAKey();
    }
    
    void UnlockStatus::saveState (const String&)
    {
    
    }
    
    String UnlockStatus::getState()
    {
        return String();
    }
    
    String UnlockStatus::getWebsiteName()
    {
        return "kushview.net";
    }
    
    URL UnlockStatus::getServerAuthenticationURL()
    {
        return URL("https://kushview.net/blahblahblah");
    }
    
    String UnlockStatus::readReplyFromWebserver (const String& email, const String& password)
    {
        return String();
    }
    
    StringArray UnlockStatus::getLocalMachineIDs()
    {
        return OnlineUnlockStatus::getLocalMachineIDs();
    }
}
