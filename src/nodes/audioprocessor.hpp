// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/processor.hpp>

namespace element {

class MidiPipe;

class AudioProcessorNode : public Processor,
                           public juce::AudioProcessorListener
{
public:
    AudioProcessorNode (uint32_t nodeId, juce::AudioProcessor* processor);
    AudioProcessorNode (juce::AudioProcessor*);

    virtual ~AudioProcessorNode();

    /** Returns the processor as an AudioProcessor */
    juce::AudioProcessor* getAudioProcessor() const noexcept override { return proc.get(); }

    void setPlayHead (juce::AudioPlayHead* playhead) override
    {
        Processor::setPlayHead (playhead);
        if (auto* p = proc.get())
            p->setPlayHead (playhead);
    }

    void getState (juce::MemoryBlock&) override;
    void setState (const void*, int) override;

    void prepareToRender (double sampleRate, int maxBufferSize) override;
    void releaseResources() override;
    void refreshPorts() override;

    void getPluginDescription (juce::PluginDescription& desc) const override;
    bool wantsContext() const noexcept override { return false; }

    void audioProcessorChanged (juce::AudioProcessor*, const ChangeDetails&) override;
    void audioProcessorParameterChanged (juce::AudioProcessor*, int, float) override {}

protected:
    ParameterPtr getParameter (const PortDescription& port) override;

private:
    std::unique_ptr<juce::AudioProcessor> proc;
    juce::Atomic<int> enabled { 1 };
    juce::MemoryBlock pluginState;
    ParameterArray params;

    struct EnablementUpdater : public juce::AsyncUpdater
    {
        EnablementUpdater (AudioProcessorNode& n) : node (n) {}
        ~EnablementUpdater() {}
        void handleAsyncUpdate() override;
        AudioProcessorNode& node;
    } enablement;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioProcessorNode)
};

} // namespace element
