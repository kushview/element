// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/core.hpp>
#include <element/juce/graphics.hpp>

namespace element {

/** Information used in the About screen. */
struct AboutInfo {
    /** The application's title. */
    juce::String title;
    /** The application's version. */
    juce::String version;
    /** Copyright notice. */
    juce::String copyright;
    /** EULA text. */
    juce::String licenseText;
    /** Link to show. */
    juce::URL link;
    /** Text of link button. */
    juce::String linkText;
    /** App icon or logo. */
    juce::Image logo;
};

} // namespace element
