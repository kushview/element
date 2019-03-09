#include "Tests.h"

/* 
    !!! IMPORTANT !!!!
    These tests will FAIL if you do not have a local kushview.net running
    and seeded with the appropriate license keys and products in the database.
*/

namespace Element {

#if EL_USE_LOCAL_AUTH

static RSAKey testPrivateKey()
{
    RSAKey key (EL_PRIVKEY);
    return key;
}

class LicenseActivation : public UnitTestBase
{
public:
    LicenseActivation() : UnitTestBase ("License Activation", "auth", "activate") { }
    
    void initialise()   override  { initializeWorld(); }
    void shutdown()     override  { shutdownWorld(); }

   #if defined (EL_SOLO)
    void runTest() override
    {
        StringPairArray params;
        params.set ("price_id", String (EL_SOLO_PRICE_ID));

        beginTest ("solo initially locked");
        auto& status = getWorld().getUnlockStatus();
        expect (false == (bool) status.isUnlocked());

        OnlineUnlockStatus::UnlockResult result;

        beginTest ("solo unlocked with trial license & is expiring");
        result = status.activateLicense (EL_LICENSE_TRIAL, {}, {}, params);
        status.save(); status.loadAll();
        expect (result.succeeded);
        expect ((bool) status.isExpiring());
        expect ((bool) status.isTrial());
        expect (status.getExpiryTime() > Time::getCurrentTime());

        beginTest ("solo locked with expired trial license");
        result = status.activateLicense (EL_LICENSE_TRIAL_EXPIRED, {}, {}, params);
        status.save(); status.loadAll();
        expect (! result.succeeded);
        expect (! (bool) status.isUnlocked());
        expect ((bool) status.isTrial());
        expect (! (bool) status.isExpiring());
        expect (status.getExpiryTime() <= Time());
        
        beginTest ("solo unlocks with solo license");
        result = status.activateLicense (EL_LICENSE_SOLO, {}, {}, params);
        status.save(); status.loadAll();
        expect (result.succeeded);
        expect (! (bool) status.isExpiring());
        expect (! (bool) status.isTrial());
        expect ((bool) status.isUnlocked());
        expect ((bool) status.isFullVersion());

        beginTest ("solo unlocks with pro license");
        result = status.activateLicense (EL_LICENSE_PRO, {}, {}, params);
        status.save(); status.loadAll();
        expect (result.succeeded);
        expect (! (bool) status.isExpiring());
        expect (! (bool) status.isTrial());
        expect ((bool) status.isUnlocked());
        expect ((bool) status.isFullVersion());
    }

   #elif defined (EL_PRO)
    void runTest() override
    {
        StringPairArray params;
        params.set ("price_id", String (EL_PRO_PRICE_ID));

        beginTest ("pro initially locked");
        auto& status = getWorld().getUnlockStatus();
        expect (false == (bool) status.isUnlocked());

        OnlineUnlockStatus::UnlockResult result;

        beginTest ("pro unlocked with trial license & is expiring");
        result = status.activateLicense (EL_LICENSE_TRIAL, {}, {}, params);
        status.save(); status.loadAll();
        expect (result.succeeded, result.errorMessage);
        expect ((bool) status.isExpiring());
        expect ((bool) status.isTrial());
        expect (status.getExpiryTime() > Time::getCurrentTime());

        beginTest ("pro locked with expired trial license");
        result = status.activateLicense (EL_LICENSE_TRIAL_EXPIRED, {}, {}, params);
        status.save(); status.loadAll();
        DBG("response message: " << result.errorMessage);
        expect (! result.succeeded);
        expect (! (bool) status.isUnlocked());
        expect ((bool) status.isTrial());
        expect (! (bool) status.isExpiring());
        expect (status.getExpiryTime() <= Time());

        beginTest ("pro locked with solo license");
        result = status.activateLicense (EL_LICENSE_SOLO, {}, {}, params);
        status.save(); status.loadAll();
        DBG("response message: " << result.errorMessage);
        expect (! result.succeeded);
        expect (! (bool) status.isExpiring());
        expect (! (bool) status.isTrial());
        expect (! (bool) status.isUnlocked());
        expect (! (bool) status.isFullVersion());

        beginTest ("pro unlocks with pro license");
        result = status.activateLicense (EL_LICENSE_PRO, {}, {}, params);
        status.save(); status.loadAll();
        expect (result.succeeded, result.errorMessage);
        expect (! (bool) status.isExpiring());
        expect (! (bool) status.isTrial());
        expect ((bool) status.isUnlocked());
        expect ((bool) status.isFullVersion());
    }
   #endif
};

static LicenseActivation sLicenseActivation;


class OfflineLicenseRequest : public UnitTestBase
{
public:
    OfflineLicenseRequest() : UnitTestBase ("License Activation", "auth", "request") { }
    
    void initialise()   override  { initializeWorld(); }
    void shutdown()     override  { shutdownWorld(); }

    void runTest() override
    {
        beginTest ("license request");
    }
};

#endif

}
