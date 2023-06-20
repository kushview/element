// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/core.hpp>
#include <element/juce/graphics.hpp>

namespace element {

struct AboutInfo {
    juce::String title;
    juce::String version;
    juce::String copyright;
    juce::String licenseText;
    juce::URL link;
    juce::String linkText;
    juce::Image logo;
};

} // namespace element
