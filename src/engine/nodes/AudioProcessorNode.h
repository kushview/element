
#pragma once

#include "engine/GraphNode.h"

namespace Element {

class GraphProcessor;
class MidiPipe;

class AudioProcessorNode : public GraphNode
{
public:
    AudioProcessorNode (uint32 nodeId, AudioProcessor* processor);
    virtual ~AudioProcessorNode();

    /** Returns the processor as an AudioProcessor */
    AudioProcessor* getAudioProcessor() const noexcept override { return proc; }
    
    void getState (MemoryBlock&) override;
    void setState (const void*, int) override;
    
protected:
    void createPorts() override;

#if 0
    /** If an audio plugin instance, fill the details */
    void getPluginDescription (PluginDescription& desc);
    
    /** Returns true if the processor is suspended */
    bool isSuspended() const;
    
    /** Suspend processing */
    void suspendProcessing (const bool);

    inline bool wantsMidiPipe() const override { return false; }
    
    /** Enable or disable this node */
    void setEnabled (const bool shouldBeEnabled);

    /** Returns true if this node is enabled */
    inline bool isEnabled()  const { return enabled.get() == 1; }

protected:
    AudioProcessorNode (uint32 nodeId, AudioProcessor*) noexcept;
    virtual void createPorts();
    kv::PortList ports;
#endif

private:
    ScopedPointer<AudioProcessor> proc;
    Atomic<int> enabled { 1 };
    MemoryBlock pluginState;

    struct EnablementUpdater : public AsyncUpdater
    {
        EnablementUpdater (AudioProcessorNode& n) : node (n) { }
        ~EnablementUpdater() { }
        void handleAsyncUpdate() override;
        AudioProcessorNode& node;
    } enablement;

#if 0
    friend class GraphProcessor;
    friend class GraphController;
    friend class EngineController;
    friend class Node;
    bool isPrepared = false;    
    void setParentGraph (GraphProcessor*);
    void prepare (double sampleRate, int blockSize, GraphProcessor*, bool willBeEnabled = false);
    void unprepare();
    void resetPorts();
    
    Atomic<float> gain, lastGain, inputGain, lastInputGain;
    OwnedArray<AtomicValue<float> > inRMS, outRMS;
    
    GraphProcessor* parent;
    Atomic<int> keyRangeLow { 0 };
    Atomic<int> keyRangeHigh { 127 };
    Atomic<int> transposeOffset { 0 };
    MidiChannels midiChannels;

    CriticalSection propertyLock;

#endif
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioProcessorNode);
};

}
