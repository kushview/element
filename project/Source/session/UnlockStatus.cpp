
#include "session/UnlockStatus.h"
#include "Globals.h"
#include "Settings.h"

#define EL_LICENSE_SETTINGS_KEY "L"

#if EL_USE_LOCAL_AUTH
 #define EL_PRODUCT_ID "15"
 #define EL_BASE_URL "http://kushview.dev"
 #define EL_AUTH_URL EL_BASE_URL "/edd-cp"
 #define EL_PUBKEY "3,753d95fa8511b392b09e5800043d41d1a7b2d330705f5714dcf2b31c8e22a7e9"
 #define EL_PRIVKEY "4e290ea703612261cb143aaaad7e2be03282bec968eea82b7d064226e66321ab,753d95fa8511b392b09e5800043d41d1a7b2d330705f5714dcf2b31c8e22a7e9"
 #define EL_LICENSE_1 "5df8e7d22e79d40f10ba406eecb52e39"
 #define EL_LICENSE_2 "010157234b4901078d29618473bde09a"
 #define EL_LICENSE_3 "15311c04ee9991ae7ef698a30b0a789c"
 #define EL_LICENSE_4 "b2dbbd725140bd7cbb4da9f3975740fb"

#elif EL_USE_CI_AUTH
 #define EL_PRODUCT_ID "12"
 #define EL_BASE_URL "https://ci.kushview.net"
 #define EL_AUTH_URL EL_BASE_URL "/edd-cp"
 #define EL_PUBKEY "5,7bc48fe0cef16975604686123c4b8c9f597b8d62b839e14f6300e632f993d613c406c068ecccd912c845ab314574ae727d55ef1ce8257e6d6dfd239a1cf5831753632a8eb9615d1033264e132edfcf537bc1e643288f45138e364fb2e2afe91c43ceaf929209d3d26428f6f276242b8505e63a923702f3990000fa043a324473"

#else
 #define EL_PRODUCT_ID "20"
 #define EL_BASE_URL "https://kushview.net"
 #define EL_AUTH_URL EL_BASE_URL "/products/authorize"
 #define EL_PUBKEY "5,c72ce63fbe64d394711f0623ee2efa749f59e192cc87ed440bab12d9f2c8cc67e0464ad18b483c171e8e9762e1a14106348f633e7acde5ec9271e9927582df02816b65d3f836d5a7a46baa3af530adec166fdc0c68320ba68d5e21ca493772753c834388fbf7d21f3e38bc3b8b7ac917cb396c1579ce9e347215fc1b1e837d099f46d9a8c422369f64d93c3f18d85e20895f244192abc1919da275eb3cebc15655f2be9e9ee97298fb8fd21c33b01d9f1849ac6ba2e1d91852847a675c6f4e2c7f00d6f4f29db9357c8fdf4af9484da07018ab986ee6ad9c88ba5a724cb98751c6c0ccbbd94c9d70a90d3cec7f8f9a42463f1df6c342c5f2dd272cf112c204f5"
#endif

namespace Element {
    const char* UnlockStatus::propsKey = "props";
    const char* UnlockStatus::fullKey = "f";
    const char* UnlockStatus::priceIdKey = "price_id";
    
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
    
    static URL elGetProUpgradeUrl()
    {
        return URL (EL_BASE_URL "/products/element?pro_upgrade=1");
    }
    
    void UnlockStatus::launchProUpgradeUrl()
    {
        elGetProUpgradeUrl().launchInDefaultBrowser();

//        // http://kushview.dev/checkout/?edd_action=sl_license_upgrade&license_id=129&upgrade_id=3
//        URL url (EL_BASE_URL "/checkout/");
//        url.withParameter("edd_action", "sl_license_upgrade")
//           .withParameter("license_id", "129")
//           .withParameter("upgrade_id", "3")
//           .launchInDefaultBrowser();
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
    
    StringArray UnlockStatus::getLocalMachineIDs()
    {
        auto ids (OnlineUnlockStatus::getLocalMachineIDs());
        return ids;
    }
}
