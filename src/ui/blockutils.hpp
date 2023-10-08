// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <string>
#include <vector>

#include <element/node.hpp>

#include "nodes/audioprocessor.hpp"
#include "ui/block.hpp"

namespace element {
namespace detail {

// true if it's an audio mixer.
static inline bool isAudioMixer (const Node& node)
{
    return node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_AUDIO_MIXER);
}

// true if the node is a Midi Device (i.e. not global MIDI)
static inline bool isMidiDevice (const Node& node)
{
    return node.isMidiInputDevice() || node.isMidiOutputDevice();
}

// true if the node probably supports embedding outside a plugin window.
static inline bool supportsEmbed (const Node& node)
{
    if (node.isIONode() || node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_PLACEHOLDER))
        return false;

#if JUCE_LINUX
    if (node.getFormat() == "LV2")
        return false;
#endif

    // Allow embedding at first to expose problems.
    return true;
}

// true if the node supports Audio bus config.
static inline bool supportsAudioBuses (const Node& node)
{
    const std::vector<std::string> supported = { "AU", "VST3", "VST" };
    if (supported.end() != std::find (supported.begin(), supported.end(), node.getFormat().toString()))
        return true;

    if (isMidiDevice (node) || node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_PLACEHOLDER))
        return false;

    if (auto o = node.getObject())
        if (nullptr != dynamic_cast<AudioProcessorNode*> (o))
            return true;

    return false;
}

// Update button status for normal blocks.
static inline void updateBlockButtonVisibility (BlockComponent& block, const Node& node)
{
    if (block.getDisplayMode() == BlockComponent::Compact || block.getDisplayMode() == BlockComponent::Small)
    {
        block.setMuteButtonVisible (false);
        block.setConfigButtonVisible (false);
        block.setPowerButtonVisible (false);
    }
    else
    {
        block.setMuteButtonVisible (true);
        block.setConfigButtonVisible (true);
        block.setPowerButtonVisible (true);

        if (node.isIONode() || node.isRootGraph() || detail::isMidiDevice (node))
        {
            block.setMuteButtonVisible (false);
            block.setPowerButtonVisible (false);
        }

        if (detail::supportsAudioBuses (node))
        {
            block.setConfigButtonVisible (true);
        }
        else
        {
            block.setConfigButtonVisible (false);
        }
    }
}

} // namespace detail
} // namespace element
