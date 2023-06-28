/*
    This file is part of Element
    Copyright (C) 2014-2020  Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

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
