// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include "./pluginconfig.h"
#include "pluginprocessor.hpp"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new element::PluginProcessor (
        element::PluginProcessor::Effect, 16);
}
