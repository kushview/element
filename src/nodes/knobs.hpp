// Copyright 2019-2023 Kushview, LLC <info@kushview.net>
// Author: Jatin Chowdhury (jatin@ccrma.stanford.edu)
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce/gui_basics.hpp>
#include <element/juce/audio_processors.hpp>

namespace element {

class KnobsComponent : public juce::Component
{
public:
    KnobsComponent (juce::AudioProcessor& proc, std::function<void()> paramLambda = {});
    ~KnobsComponent() {}

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::OwnedArray<juce::Slider> sliders;
    juce::OwnedArray<juce::ComboBox> boxes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KnobsComponent)
};

} // namespace element
