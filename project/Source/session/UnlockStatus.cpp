
#include "session/UnlockStatus.h"
#include "Globals.h"
#include "Settings.h"

#define EL_PRODUCT_ID "net.kushview.Element"
#define EL_LICENSE_SETTINGS_KEY "L"

#if EL_USE_LOCAL_AUTH
 #define EL_AUTH_URL "http://kushview.dev/products/authorize"
 #define EL_PUBKEY "11,785384ff65886b9e03e5c6c12bf46bace50be4e8183473f9a7f1fdf6a6c53445f1ea296f75576c72061e5822b70f0a8dc8fe1901f34e3eeb83f2013aa89a88fbd3f0ec68c6057019d397a50d9818f473cb99aea1d5aa1af1452e45095e5602a73112883364d2fa1fff872c9109b0e3436a881463b02a76e525b9c72dd8088dbbe3048d4b856e3623c4968200986840f650878851d03ee3cd43166f595ec121b11f46819a280864f941dd89e1b125f8b9c87dc11e7a76c92f13e6405242ba791ec19a0346f2c064bcea495da1268c567b8f6d67fad140c3069aa42f6005f7edf0181a226cfe18acf942adf72a4eb678bf142341041b06cf5d26458cbac77c75c5"

#else
 #define EL_AUTH_URL "https://kushview.net/products/authorize"
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
    
    void UnlockStatus::saveState (const String& state)
    {
        if (auto* const props = settings.getUserSettings())
            props->setValue (EL_LICENSE_SETTINGS_KEY, state);
    }
    
    String UnlockStatus::getState()
    {
        if (auto* const props = settings.getUserSettings())
            return props->getValue (EL_LICENSE_SETTINGS_KEY);
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
        const URL url (getServerAuthenticationURL()
            .withParameter ("product", getProductID())
            .withParameter ("email", email)
            .withParameter ("password", password)
            .withParameter ("os", SystemStats::getOperatingSystemName())
            .withParameter ("mach", getLocalMachineIDs().joinIntoString(",")));
        
        DBG ("Trying to unlock via: " << url.toString (true));
        return url.readEntireTextStream();
    }
    
    StringArray UnlockStatus::getLocalMachineIDs()
    {
        auto ids (OnlineUnlockStatus::getLocalMachineIDs());
        return ids;
    }
}
