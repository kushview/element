// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/core.hpp>

namespace element {
namespace gzip {

juce::String encode (const juce::String& input);
juce::String decode (const juce::String& input);

} // namespace gzip
} // namespace element
