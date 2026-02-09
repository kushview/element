// SPDX-FileCopyrightText: 2026 Kushview, LLC
// SPDX-License-Identifier: ISC

#include <winsparkle.h>

#include <element/version.h>
#include <element/juce/core.hpp>
#include <element/application.hpp>
#include <element/ui/updater.hpp>

#ifndef ELEMENT_APPCAST_URL
#define ELEMENT_APPCAST_URL "http://localhost:8000/appcast.xml"
#endif

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
        const juce::String versionString (ELEMENT_VERSION_STRING);
        const juce::String buildVersion (ELEMENT_BUILD_VERSION);
        win_sparkle_set_app_details (L"Kushview", L"Element", versionString.toWideCharPointer());
        win_sparkle_set_app_build_version (buildVersion.toWideCharPointer());
        win_sparkle_set_appcast_url (ELEMENT_APPCAST_URL);
        win_sparkle_set_automatic_check_for_updates (0);

        win_sparkle_set_can_shutdown_callback (canShutdown);
        win_sparkle_set_shutdown_request_callback (shutdownRequest);

#ifdef ELEMENT_EDDSA_PUBLIC_KEY
        win_sparkle_set_eddsa_public_key (ELEMENT_EDDSA_PUBLIC_KEY);
#endif
    }

    void check (bool async) override
    {
        juce::ignoreUnused (async);
        if (! _initialized)
        {
            win_sparkle_init();
            _initialized = true;
        }

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
