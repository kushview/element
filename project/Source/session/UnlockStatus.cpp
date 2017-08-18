
#include "session/UnlockStatus.h"
#include "Globals.h"
#include "Settings.h"

#define EL_PRODUCT_ID "net.kushview.Element"
#define EL_LICENSE_SETTINGS_KEY "L"

#if EL_USE_LOCAL_AUTH
 #define EL_BASE_URL "http://kushview.dev"
 #define EL_AUTH_URL EL_BASE_URL "/juce/authorize"
 #define EL_PUBKEY "5,b2edd6e3c5e596716bc7323d07faca55"

#else
 #define EL_BASE_URL "https://kushview.net"
 #define EL_AUTH_URL EL_BASE_URL "/products/authorize"
 #define EL_PUBKEY "5,d5460977fa4b1aacdc00ec5cff98e95a98abac0d5d2cbfb9c50ff254f870fc95e930cb563aa7b344a7d42eb804bd09694c8bbb6f4fc41baf0b5e7b30073d6c0577b413e7aa71dcaf6f149689ae418a9c34f6b0a1b539d81bc3518d260c9dc8f5a6211c06b523bc4929381333b2e1f0d28ad04fa3dbe536610d7db1a7e51d4ae5f8624b924a64f0691687881f854ec46a702fe21323d46eef7c8adf43f96813fb60d43c551cb82ddc59feb606a9f2894722a2f45de71afe36abe961d83368c9a93b16705695a499e87c0c0a95fc43dac9d27aac18a78c1fdec659aeadbf5a6270b6ee407eeffee882311ffbc1ed3905ab1fbae5536a49275f55d7b753644f6cd5"
#endif

namespace Element {
    UnlockStatus::UnlockStatus (Globals& g) : settings (g.getSettings()) { }
    String UnlockStatus::getProductID() { return EL_PRODUCT_ID; }
    bool UnlockStatus::doesProductIDMatch (const String& returnedIDFromServer)
    {
        return getProductID() == returnedIDFromServer;
    }
    
    RSAKey UnlockStatus::getPublicKey() { return RSAKey (EL_PUBKEY); }
    
    void UnlockStatus::saveState (const String& data)
    {
        if (auto* const props = settings.getUserSettings())
            props->setValue (EL_LICENSE_SETTINGS_KEY, data);
    }
    
    String UnlockStatus::getState()
    {
        if (auto* const props = settings.getUserSettings())
        {
            const String value =  props->getValue (EL_LICENSE_SETTINGS_KEY).toStdString();
            eddRestoreState (value);
            return value;
        }
        return String();
    }
    
    String UnlockStatus::getWebsiteName() {
        return "Kushview";
    }
    
    URL UnlockStatus::getServerAuthenticationURL() {
        const URL authurl (EL_AUTH_URL);
        return authurl;
    }
    
    URL UnlockStatus::getApiEndPoint() {
        return URL (EL_BASE_URL);
    }
    
    StringArray UnlockStatus::getLocalMachineIDs()
    {
        auto ids (OnlineUnlockStatus::getLocalMachineIDs());
        return ids;
    }
}
