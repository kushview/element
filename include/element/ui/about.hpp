#pragma once

#include <element/juce/core.hpp>
#include <element/juce/graphics.hpp>

namespace element {

struct AppInfo {
    juce::String title;
    juce::String version;
    juce::String copyright;
    juce::String licenseText;
    juce::URL link;
    juce::String linkText;
    juce::Image logo;
};

} // namespace element
