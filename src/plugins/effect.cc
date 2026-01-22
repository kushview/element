// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pluginprocessor.hpp"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    // Ensure JUCE's MessageManager is initialized before creating processor
    // This initializes critical JUCE statics like AudioChannelSet
    juce::MessageManager::getInstance();
    
    return new element::PluginProcessor (
        element::PluginProcessor::Effect, 16);
}

#if ELEMENT_UPDATER
#include "./pluginupdater.cc"
#endif
