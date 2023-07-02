// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include "./pluginconfig.h"
#include "../pluginprocessor.cpp"
#include "../plugineditor.cpp"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new element::PluginProcessor (
        element::PluginProcessor::MidiEffect, 0);
}
