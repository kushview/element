// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once
#include <element/juce/config.h>
#include <juce_core/juce_core.h>

namespace element {
// FIXME:
using int32 = juce::int32;
using uint8 = juce::uint8;
using uint16 = juce::uint16;
using uint32 = juce::uint32;

using juce::Array;
using juce::File;
using juce::Identifier;
using juce::String;
using juce::StringArray;

template <typename Dat>
using HeapBlock = juce::HeapBlock<Dat>;
} // namespace element
