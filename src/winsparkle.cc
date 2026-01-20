// SPDX-FileCopyrightText: 2026 Kushview, LLC
// SPDX-License-Identifier: ISC

#include <iostream>

#include <winsparkle.h>

#include <element/version.h>
#include <element/juce/core.hpp>
#include <element/ui/updater.hpp>

namespace element {

class WinSparkleUpdater : public Updater
{
public:
    WinSparkleUpdater()
    {
        std::string ver = ELEMENT_VERSION_STRING;
        std::wstring wver (ver.begin(), ver.end());
        win_sparkle_set_app_details (L"Kushview", L"Element", wver.c_str());
        win_sparkle_set_app_build_version (L"100");
        win_sparkle_set_appcast_url ("http://localhost:8080/appcast.xml");
        win_sparkle_set_automatic_check_for_updates (0);
        win_sparkle_set_eddsa_public_key ("pubkey");
    }

    void check (bool async) override
    {
        juce::ignoreUnused (async);
        if (! _initialized) {
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