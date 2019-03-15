#pragma once

#include "ElementApp.h"

namespace Element {

class GraphProcessor;
class MidiPipe;

class GraphNode : public ReferenceCountedObject
{
public:

    /** Special parameter indexes when mapping universal node settings */
    enum SpecialParameter
    {
        NoParameter             = -1,
        EnabledParameter        = -2,
        BypassParameter         = -3,
        SpecialParameterBegin   = BypassParameter,
        SpecialParameterEnd     = NoParameter
    };

    /** The ID number assigned to this node. This is assigned by the graph
        that owns it, and can't be changed. */
    const uint32 nodeId;

    virtual ~GraphNode();
    
    /** Create a node suitable for binding to a root graph */
    static GraphNode* createForRoot (GraphProcessor*);
    
    /** Returns an audio processor if available */
    JUCE_DEPRECATED_WITH_BODY(virtual AudioProcessor* getAudioProcessor() const noexcept, { return nullptr; })
   
    /** The actual processor object dynamic_cast'd to T */
    template<class T> inline T* processor() const noexcept { return dynamic_cast<T*> (getAudioProcessor()); }

    /** Returns the processor as an Audio Plugin Instance */
    AudioPluginInstance* getAudioPluginInstance() const noexcept { return processor<AudioPluginInstance>(); }

    virtual void prepareToRender (double sampleRate, int maxBufferSize) = 0;
    virtual void releaseResources() = 0;

    virtual bool wantsMidiPipe() const { return false; }
    virtual void render (AudioSampleBuffer&, MidiPipe&) { }
    virtual void renderBypassed (AudioSampleBuffer&, MidiPipe&);
    
    /** Returns the total number of audio inputs */
    int getNumAudioInputs() const;

    /** Returns the total number of audio ouputs */
    int getNumAudioOutputs() const;
    
    /** Returns the type of port
        
        @param port The port to check
     */
    PortType getPortType (const uint32 port) const;

    /** Returns the total number of ports on this node */
    uint32 getNumPorts() const;

    /** Returns the number of ports for a given type and flow */
    int getNumPorts (const PortType type, const bool isInput) const;
    
    /** Returns the default MIDI input port */
    uint32 getMidiInputPort() const;

    /** Returns the default MIDI output port */
    uint32 getMidiOutputPort() const;

    /** Returns a channel index for a port
     
        @param port The port to find.
    */
    int getChannelPort (const uint32 port) const;

    /** Returns a port index for a channel of given type
     
        @param type The port type to find
        @param channel The channel to find
        @param isInput True if for an input port
    */
    uint32 getPortForChannel (const PortType type, const int channel, const bool isInput) const;

    int getNthPort (const PortType type, const int channel, bool, bool) const;
    
    /** Returns true if an input port */
    bool isPortInput (const uint32 port) const;

    /** Returns true if an output port */
    bool isPortOutput (const uint32 port) const;
    
    /** Returns true if the underyling processor is a SubGraph or Graph */
    bool isGraph() const noexcept;

    /** Returns true if the processor is a root graph */
    bool isRootGraph() const noexcept;

    /** Returns true if the processor is a subgraph */
    bool isSubGraph() const noexcept;
    
    /** Get the type string for this Node */
    const String& getTypeString() const;

    /** Returns true if the parameter is valid.
        Valid = special param index or positive and below
                the number of params provided by the implementation
     */
    bool containsParameter (const int index) const;

    /** If an audio plugin instance, fill the details */
    void getPluginDescription (PluginDescription& desc);

    /** Returns true if the processor is suspended */
    bool isSuspended() const;
  
    /** Suspend processing */
    void suspendProcessing (const bool);

    /** Get latency audio samples */
    int getLatencySamples() const { return latencySamples; }

    /** Set latency samples */
    void setLatencySamples (int latency) { if (latencySamples != latency) latencySamples = latency; }

    /** Set the Input Gain of this Node */
    void setInputGain (const float f);

    /** Set the Gain of this Node */
    void setGain (const float f);

    inline float getInputGain() const { return inputGain.get(); }
    inline float getGain() const { return gain.get(); }
    inline float getLastGain() const { return lastGain.get(); }
    inline float getLastInputGain() const { return lastInputGain.get(); }

    inline void updateGain() noexcept
    {
        if (lastGain.get() != gain.get())
            lastGain = gain;
        if (lastInputGain.get() != inputGain.get())
            lastInputGain = inputGain;
    }

    ValueTree getMetadata() const { return metadata; }

    bool isAudioIONode() const;
    bool isAudioInputNode() const;
    bool isAudioOutputNode() const;
    bool isMidiIONode() const;

    /* returns the parent graph. If one has not been set, then
       this will return nullptr */
    GraphProcessor* getParentGraph() const;

    void setInputRMS (int chan, float val);
    float getInputRMS(int chan) const { return (chan < inRMS.size()) ? inRMS.getUnchecked(chan)->get() : 0.0f; }
    void setOutputRMS (int chan, float val);
    float getOutputRMS (int chan) const { return (chan < outRMS.size()) ? outRMS.getUnchecked(chan)->get() : 0.0f; }

    void connectAudioTo (const GraphNode* other);

    /** Enable or disable this node */
    void setEnabled (const bool shouldBeEnabled);

    /** Returns true if this node is enabled */
    inline bool isEnabled()  const { return enabled.get() == 1; }

    inline void setKeyRange (const int low, const int high)
    {
        jassert (low <= high);
        jassert (isPositiveAndBelow (low, 128));
        jassert (isPositiveAndBelow (high, 128));
        keyRangeLow.set (low); keyRangeHigh.set (high);
    }

    inline void setKeyRange (const Range<int>& range) { setKeyRange (range.getStart(), range.getEnd()); }

    inline Range<int> getKeyRange() const { return Range<int> { keyRangeLow.get(), keyRangeHigh.get() }; }

    inline void setTransposeOffset (const int value)
    {
        jassert (value >= -24 && value <= 24);
        transposeOffset.set (value);
    }

    inline int getTransposeOffset() const { return transposeOffset.get(); }

    const CriticalSection& getPropertyLock() const { return propertyLock; }

    inline bool hasMidiProgram() const { return isPositiveAndBelow (midiProgram.get(), 128); }
    inline int getMidiProgram() const { return midiProgram.get(); }
    inline void setMidiProgram (const int program)
    {
        if (program < -1 || program > 127) {
            jassertfalse; // out of range
            return;
        }
        midiProgram.set (program);
    }
    void reloadMidiProgram();

    inline void setMidiChannels (const BigInteger& ch)
    {
        ScopedLock sl (propertyLock);
        midiChannels.setChannels (ch);
    }

    inline const MidiChannels& getMidiChannels() const { return midiChannels; }


    inline virtual int getNumPrograms() const
    { 
        if (auto* const proc = getAudioProcessor())
            return proc->getNumPrograms();
        return 1;
    }

    inline virtual int getCurrentProgram() const
    {
        if (auto* const proc = getAudioProcessor())
            return proc->getCurrentProgram();
        return 0;
    }

    inline virtual const String getProgramName (int index) const
    {
        if (auto* const proc = getAudioProcessor())
            return proc->getProgramName (index);
        return "Program " + String (index + 1);
    }

    inline virtual void setCurrentProgram (int index)
    {
        if (auto* const proc = getAudioProcessor())
            return proc->setCurrentProgram (index);
    }

    virtual void getState (MemoryBlock&) = 0;
    virtual void setState (const void*, int sizeInBytes) = 0;

    /** Triggered when the enabled state changes */
    Signal<void(GraphNode*)> enablementChanged;

    /** Triggered when the bypass state changes */
    Signal<void(GraphNode*)> bypassChanged;

protected:
    GraphNode (uint32 nodeId) noexcept;
    virtual void createPorts() = 0;
    kv::PortList ports;
    ValueTree metadata;

private:
    friend class GraphProcessor;
    friend class GraphManager;
    friend class EngineController;
    friend class Node;
    
    GraphProcessor* parent = nullptr;
    bool isPrepared = false;
    Atomic<int> enabled { 1 };
    Atomic<int> bypassed { 0 };
    int latencySamples = 0;

    Atomic<float> gain, lastGain, inputGain, lastInputGain;
    OwnedArray<AtomicValue<float> > inRMS, outRMS;
    
    Atomic<int> keyRangeLow { 0 };
    Atomic<int> keyRangeHigh { 127 };
    Atomic<int> transposeOffset { 0 };
    MidiChannels midiChannels;
    Atomic<int> midiProgram { -1 };

    CriticalSection propertyLock;
    struct EnablementUpdater : public AsyncUpdater
    {
        EnablementUpdater (GraphNode& g) : graph (g) { }
        ~EnablementUpdater() { }
        void handleAsyncUpdate() override;
        GraphNode& graph;
    } enablement;

    struct MidiProgramLoader : public AsyncUpdater
    {
        MidiProgramLoader (GraphNode& n) : node (n) { }
        ~MidiProgramLoader() { cancelPendingUpdate(); }
        void handleAsyncUpdate() override;
        GraphNode& node;
    } midiProgramLoader;

    void setParentGraph (GraphProcessor*);
    void prepare (double sampleRate, int blockSize, GraphProcessor*, bool willBeEnabled = false);
    void unprepare();
    void resetPorts();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphNode)
};

/** A convenient typedef for referring to a pointer to a node object. */
typedef ReferenceCountedObjectPtr<GraphNode> GraphNodePtr;

}
