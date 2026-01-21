// SPDX-FileCopyrightText: 2026 Kushview, LLC
// SPDX-License-Identifier: ISC

#include <winsparkle.h>

#include <element/version.h>
#include <element/juce/core.hpp>
#include <element/application.hpp>
#include <element/ui/updater.hpp>

namespace element {

static int __cdecl canShutdown()
{
    return Application::canShutdown() ? 1 : 0;
}

static void __cdecl shutdownRequest()
{
    // Request the app to shut down for update installation
    Application::getInstance()->systemRequestedQuit();
}

class WinSparkleUpdater : public Updater
{
public:
    WinSparkleUpdater()
    {
        // Use "0.0.1" to force update detection during testing
        // win_sparkle_set_app_details (L"Kushview", L"Element", L"0.0.1");
        // win_sparkle_set_app_build_version (L"1");
        win_sparkle_set_appcast_url ("http://localhost:8000/appcast.xml");
        win_sparkle_set_automatic_check_for_updates (0);

        // Set shutdown callbacks so installer can replace the running exe
        win_sparkle_set_can_shutdown_callback (canShutdown);
        win_sparkle_set_shutdown_request_callback (shutdownRequest);

        // Don't set public key for testing - allows unsigned updates
        // win_sparkle_set_eddsa_public_key ("pubkey");
    }

    void check (bool async) override
    {
        juce::ignoreUnused (async);
        if (! _initialized)
        {
            win_sparkle_init();
            _initialized = true;
        }

        // win_sparkle_check_update_without_ui();
        win_sparkle_check_update_with_ui();
    }

private:
    bool _initialized { false };
};

std::unique_ptr<Updater> Updater::create()
{
    return std::make_unique<WinSparkleUpdater>();
}

} // namespace element