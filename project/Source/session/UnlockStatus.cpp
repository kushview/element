
#include "session/UnlockStatus.h"

#define EL_PRODUCT_ID "Element"

#if JUCE_DEBUG
 #define EL_AUTH_URL "http://kushview.dev/products/authorize"
 #define EL_PUBKEY "thtebdfjsdflkjsflksjdfljsdf,kdflskdjfsldkfjsldkfjsld;kfja;ldkfj"
#else
 #define EL_AUTH_URL "https://kushview.dev/products/authorize"
 #define EL_PUBKEY ""
#endif

namespace Element {
    String UnlockStatus::getProductID() { return EL_PRODUCT_ID; }
    bool UnlockStatus::doesProductIDMatch (const String& returnedIDFromServer)
    {
        return getProductID() == returnedIDFromServer;
    }
    
    RSAKey UnlockStatus::getPublicKey() { return RSAKey (EL_PUBKEY); }
    
    void UnlockStatus::saveState (const String&)
    {
        jassertfalse;
    }
    
    String UnlockStatus::getState()
    {
        jassertfalse;
        return String();
    }
    
    String UnlockStatus::getWebsiteName()
    {
        return "kushview.net";
    }
    
    URL UnlockStatus::getServerAuthenticationURL()
    {
        const URL authurl (EL_AUTH_URL);
        return authurl;
    }
    
    String UnlockStatus::readReplyFromWebserver (const String& email, const String& password)
    {
        URL url (getServerAuthenticationURL()
                 .withParameter ("product", getProductID())
                 .withParameter ("email", email)
                 .withParameter ("pw", password)
                 .withParameter ("os", SystemStats::getOperatingSystemName())
                 .withParameter ("mach", getLocalMachineIDs()[0]));
        
        DBG ("Trying to unlock via: " << url.toString (true));
        
        return url.readEntireTextStream();
    }
    
    StringArray UnlockStatus::getLocalMachineIDs()
    {
        return OnlineUnlockStatus::getLocalMachineIDs();
    }
}
