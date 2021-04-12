/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

#include "engine/NodeObject.h"

namespace Element {

class MidiPipe;

class AudioProcessorNode : public NodeObject
{
public:
    AudioProcessorNode (uint32 nodeId, AudioProcessor* processor);
    virtual ~AudioProcessorNode();

    /** Returns the processor as an AudioProcessor */
    AudioProcessor* getAudioProcessor() const noexcept override { return proc.get(); }
    
    void getState (MemoryBlock&) override;
    void setState (const void*, int) override;
    
    void prepareToRender (double sampleRate, int maxBufferSize) override;
    void releaseResources() override;
    void refreshPorts() override;

protected:
    Parameter::Ptr getParameter (const PortDescription& port) override;
    
private:
    std::unique_ptr<AudioProcessor> proc;
    Atomic<int> enabled { 1 };
    MemoryBlock pluginState;
    ParameterArray params;

    struct EnablementUpdater : public AsyncUpdater
    {
        EnablementUpdater (AudioProcessorNode& n) : node (n) { }
        ~EnablementUpdater() { }
        void handleAsyncUpdate() override;
        AudioProcessorNode& node;
    } enablement;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioProcessorNode);
};

}
