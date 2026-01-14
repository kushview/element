// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce.hpp>
#include <element/porttype.hpp>
#include <element/signals.hpp>
#include <element/element.hpp>
#include <element/tags.hpp>

#include "datapath.hpp"

namespace element {
using namespace juce;

inline static bool canConnectToWebsite (const juce::URL& url, const int timeout = 2000)
{
    int status = -1;
    auto options = juce::URL::InputStreamOptions (URL::ParameterHandling::inAddress)
                       .withHttpRequestCmd ("GET")
                       .withStatusCode (&status)
                       .withConnectionTimeoutMs (timeout);
    std::unique_ptr<juce::InputStream> in (url.createInputStream (options));
    return in != nullptr;
}

inline static bool areMajorWebsitesAvailable()
{
    const char* urlsToTry[] = {
        "http://google.com", "http://bing.com", "http://amazon.com", "https://google.com", "https://bing.com", "https://amazon.com", nullptr
    };

    for (const char** url = urlsToTry; *url != nullptr; ++url)
        if (canConnectToWebsite (juce::URL (*url)))
            return true;

    return false;
}

} // namespace element
