// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/processor.hpp>

namespace element {

class MidiPipe;

class AudioProcessorNode : public Processor
{
public:
    AudioProcessorNode (uint32 nodeId, AudioProcessor* processor);
    AudioProcessorNode (AudioProcessor*);

    virtual ~AudioProcessorNode();

    /** Returns the processor as an AudioProcessor */
    AudioProcessor* getAudioProcessor() const noexcept override { return proc.get(); }

    void setPlayHead (AudioPlayHead* playhead) override
    {
        Processor::setPlayHead (playhead);
        if (auto* p = proc.get())
            p->setPlayHead (playhead);
    }

    void getState (MemoryBlock&) override;
    void setState (const void*, int) override;

    void prepareToRender (double sampleRate, int maxBufferSize) override;
    void releaseResources() override;
    void refreshPorts() override;

    void getPluginDescription (PluginDescription& desc) const override;
    bool wantsContext() const noexcept override { return false; }

protected:
    ParameterPtr getParameter (const PortDescription& port) override;

private:
    std::unique_ptr<AudioProcessor> proc;
    Atomic<int> enabled { 1 };
    MemoryBlock pluginState;
    ParameterArray params;

    struct EnablementUpdater : public AsyncUpdater
    {
        EnablementUpdater (AudioProcessorNode& n) : node (n) {}
        ~EnablementUpdater() {}
        void handleAsyncUpdate() override;
        AudioProcessorNode& node;
    } enablement;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioProcessorNode)
};

} // namespace element
