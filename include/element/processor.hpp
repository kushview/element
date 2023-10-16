// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/core.hpp>
#include <element/juce/audio_processors.hpp>

#include <element/midipipe.hpp>
#include <element/oversampler.hpp>
#include <element/atomic.hpp>
#include <element/parameter.hpp>
#include <element/portcount.hpp>
#include <element/midichannels.hpp>
#include <element/signals.hpp>

namespace element {

using namespace juce;

/* So render tasks can be friends of graph node */
namespace GraphRender {
class ProcessBufferOp;
}

class Editor;
class GraphNode;
class ProcessBufferOp;

class Processor : public ReferenceCountedObject {
public:
    /** Special parameter indexes when mapping universal node settings */
    enum SpecialParameter {
        NoParameter = -1,
        EnabledParameter = -2,
        BypassParameter = -3,
        MuteParameter = -4,
        SpecialParameterBegin = MuteParameter,
        SpecialParameterEnd = NoParameter
    };

    /** The ID number assigned to this node. This is assigned by the graph
        that owns it, and can't be changed. */
    const uint32 nodeId;

    virtual ~Processor();

    /** Returns true if a parameter index is special */
    static bool isSpecialParameter (int parameter);

    /** Returns the name of a special parameter */
    static String getSpecialParameterName (int parameter);

    //=========================================================================
    /** Returns the name of this node */
    String getName() const { return name; }

    /** Returns true if this is a T type node */
    template <class T>
    bool isA() const
    {
        return nullptr != dynamic_cast<const T*> (this);
    }

    //=========================================================================
    void setRenderDetails (double newSampleRate, int newBlockSize);
    double getSampleRate() const noexcept { return sampleRate; }
    int getBlockSize() const noexcept { return blockSize; }

    //=========================================================================
    /** Returns an audio processor if available */
    virtual AudioProcessor* getAudioProcessor() const noexcept { return nullptr; }

    /** The actual processor object dynamic_cast'd to T */
    template <class T>
    inline T* processor() const noexcept
    {
        return dynamic_cast<T*> (getAudioProcessor());
    }

    /** Returns the processor as an Audio Plugin Instance */
    AudioPluginInstance* getAudioPluginInstance() const noexcept { return processor<AudioPluginInstance>(); }

    /** Set the audio play head */
    virtual void setPlayHead (AudioPlayHead* playhead) { _playhead = playhead; }
    AudioPlayHead* getPlayHead() const noexcept { return _playhead; }

    //==========================================================================
    virtual void prepareToRender (double sampleRate, int maxBufferSize) = 0;
    virtual void releaseResources() = 0;

    virtual bool wantsMidiPipe() const { return false; }
    virtual void render (AudioSampleBuffer&, MidiPipe&, AudioSampleBuffer&) {}
    virtual void renderBypassed (AudioSampleBuffer&, MidiPipe&, AudioSampleBuffer&);

    /** Returns the total number of audio inputs */
    int getNumAudioInputs() const;

    /** Returns the total number of audio ouputs */
    int getNumAudioOutputs() const;

    //=========================================================================
    const ParameterArray& getParameters (bool inputs = true) const noexcept { return inputs ? parameters : parametersOut; }
    const PatchParameterArray& getPatches() const noexcept { return _patches; }

    ParameterPtr getParameter (int index, bool isInput) const noexcept
    {
        return getParameters (isInput)[index];
    }

    ParameterPtr getParameter (int port)
    {
        auto desc = getPort (port);
        return desc.type == PortType::Control ? getParameter (desc.channel, desc.input)
                                              : nullptr;
    }

    //=========================================================================
    /** Returns the type of port
        
        @param port The port to check
     */
    PortType getPortType (const uint32 port) const;

    /** Returns the total number of ports on this node */
    uint32 getNumPorts() const;

    /** Returns a port description by index */
    PortDescription getPort (int index) const;

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

    //==========================================================================
    virtual void refreshPorts() {}

    //==========================================================================
    /** Returns true if the underyling processor is a SubGraph or Graph */
    bool isGraph() const noexcept;

    /** Returns true if the processor is a root graph */
    bool isRootGraph() const noexcept;

    /** Returns true if the processor is a subgraph */
    bool isSubGraph() const noexcept;

    /** Get the type string for this Node */
    const String getTypeString() const noexcept;

    /** Returns true if the parameter is valid.
        Valid = special param index or positive and below
                the number of params provided by the implementation
     */
    bool containsParameter (const int index) const;

    /** Fill the details... */
    virtual void getPluginDescription (PluginDescription& desc) const;

    /** Returns true if the processor is suspended */
    bool isSuspended() const;

    /** Suspend processing */
    void suspendProcessing (const bool);

    /** Get latency audio samples */
    int getLatencySamples() const;

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

    ValueTree createPortsData() const;

    bool isAudioIONode() const;
    bool isAudioInputNode() const;
    bool isAudioOutputNode() const;
    bool isMidiIONode() const;
    bool isMidiDeviceNode() const;

    /* Returns the parent graph.
       If one has not been set, then this will return nullptr.
     */
    GraphNode* getParentGraph() const;

    void setInputRMS (int chan, float val);
    float getInputRMS (int chan) const { return (chan < inRMS.size()) ? inRMS.getUnchecked (chan)->get() : 0.0f; }
    void setOutputRMS (int chan, float val);
    float getOutputRMS (int chan) const { return (chan < outRMS.size()) ? outRMS.getUnchecked (chan)->get() : 0.0f; }

    //=========================================================================
    /** Connect this node's output audio to another node's input audio */
    void connectAudioTo (const Processor* other);

    //=========================================================================
    /** Enable or disable this node */
    void setEnabled (const bool shouldBeEnabled);

    /** Returns true if this node is enabled */
    inline bool isEnabled() const { return enabled.get() == 1; }

    //=========================================================================
    inline void setKeyRange (const int low, const int high)
    {
        jassert (low <= high);
        jassert (isPositiveAndBelow (low, 128));
        jassert (isPositiveAndBelow (high, 128));
        keyRangeLow.set (low);
        keyRangeHigh.set (high);
    }

    inline void setKeyRange (const Range<int>& range) { setKeyRange (range.getStart(), range.getEnd()); }

    inline Range<int> getKeyRange() const { return Range<int> { keyRangeLow.get(), keyRangeHigh.get() }; }

    //=========================================================================
    inline void setTransposeOffset (const int value)
    {
        jassert (value >= -24 && value <= 24);
        transposeOffset.set (value);
    }

    inline int getTransposeOffset() const { return transposeOffset.get(); }

    const CriticalSection& getPropertyLock() const { return propertyLock; }

    //=========================================================================
    /** Returns the file used for the current global MIDI Program */
    File getMidiProgramFile (int program = -1) const;

    /** Returns true if this node should use global MIDI programs */
    inline bool useGlobalMidiPrograms() const { return globalMidiPrograms.get() == 1; }

    /** Change usage of global midi programs to on or off */
    inline void setUseGlobalMidiPrograms (bool use) { globalMidiPrograms.set (use ? 1 : 0); }

    /** True if MIDI programs should be loaded when Program change messages
        are received */
    inline bool areMidiProgramsEnabled() const { return midiProgramsEnabled.get() == 1; }

    /** Enable or disable changing midi programs */
    inline void setMidiProgramsEnabled (bool enabled) { midiProgramsEnabled.set (enabled ? 1 : 0); }

    /** Returns the active midi program */
    inline int getMidiProgram() const { return midiProgram.get(); }

    /** Sets the MIDI program, note that this won't load it */
    void setMidiProgram (const int program);

    /** Sets the MIDI program's name */
    void setMidiProgramName (const int program, const String& name);

    /** Gets the MIDI program's name */
    String getMidiProgramName (const int program) const;

    /** Reloads the active MIDI program */
    void reloadMidiProgram();

    /** Save the current MIDI program */
    void saveMidiProgram();

    /** Removes a MIDI Program */
    void removeMidiProgram (int program, bool global);

    /** Get all MIDI program states stored directly on the node */
    void getMidiProgramsState (String& state) const;

    /** Load all MIDI program states to be stored on the node.
        
        @param state    The state to set. If this is empty, the midi programs
                        on the node will be cleared.
     */
    void setMidiProgramsState (const String& state);

    //=========================================================================
    inline void setMidiChannels (const BigInteger& ch)
    {
        ScopedLock sl (propertyLock);
        midiChannels.setChannels (ch);
    }

    inline const MidiChannels& getMidiChannels() const { return midiChannels; }

    //=========================================================================
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

    //=========================================================================
    void setMuted (bool muted);
    bool isMuted() const { return mute.get() == 1; }
    void setMuteInput (bool shouldMuteInput) { muteInput.set (shouldMuteInput ? 1 : 0); }
    bool isMutingInputs() const { return muteInput.get() == 1; }

    //=========================================================================
    virtual void getState (MemoryBlock&) = 0;
    virtual void setState (const void*, int sizeInBytes) = 0;

    //=========================================================================
    void setOversamplingFactor (int osFactor);
    int getOversamplingFactor();

    //=========================================================================
    void setDelayCompensation (double delayMs);
    double getDelayCompensation() const;
    int getDelayCompensationSamples() const;

    //=========================================================================
    virtual bool hasEditor() { return false; }
    virtual Editor* createEditor() { return nullptr; }

    //=========================================================================
    /** Triggered when the enabled state changes */
    Signal<void (Processor*)> enablementChanged;

    /** Triggered when the bypass state changes */
    Signal<void (Processor*)> bypassChanged;

    /** Triggered when the current MIDI program changes */
    Signal<void()> midiProgramChanged;

    /** Triggered when the mute state changes */
    Signal<void (Processor*)> muteChanged;

    /** Triggered immediately before this node is removed from a graph */
    Signal<void()> willBeRemoved;

    /** Triggered when the ports have changed */
    Signal<void()> portsChanged;

    /** Triggered when the node changes its name */
    Signal<void()> nameChanged;

protected:
    Processor (uint32 nodeId) noexcept;
    Processor (const PortList& ports);

    void setPorts (const PortList&);
    virtual void initialize() {}
    void addPatch (PatchParameter* param) { _patches.add (param); }

    /** Clear the top level referenced parameters. Some node types
        can use this in their destructor if deletion order is important
        for processors/parameters
     */
    void clearParameters();

    void setName (const String& newName)
    {
        if (newName.isNotEmpty() && newName != name) {
            name = newName;
            nameChanged();
        }
    }

    //==========================================================================
    /** Set latency samples */
    void setLatencySamples (int latency);

    //==========================================================================
    virtual ParameterPtr getParameter (const PortDescription& port) { return nullptr; }

    //==========================================================================
    void triggerPortReset();

    PortList portList() const noexcept { return ports; }

private:
    friend class EngineService;
    friend class GraphRender::ProcessBufferOp;
    friend class ProcessBufferOp;
    friend class GraphManager;
    friend class GraphNode;
    friend class Node;

    PortList ports;
    GraphNode* parent = nullptr;
    bool isPrepared = false;

    Atomic<int> enabled { 1 };
    Atomic<int> bypassed { 0 };
    Atomic<int> mute { 0 };
    Atomic<int> muteInput { 0 };

    double sampleRate = 0.0;
    int blockSize = 0;
    int latencySamples = 0;
    String name;

    ParameterArray parameters, parametersOut;
    PatchParameterArray _patches;

    Atomic<float> gain, lastGain, inputGain, lastInputGain;
    OwnedArray<AtomicValue<float>> inRMS, outRMS;

    Atomic<int> keyRangeLow { 0 };
    Atomic<int> keyRangeHigh { 127 };
    Atomic<int> transposeOffset { 0 };
    MidiChannels midiChannels;

    Atomic<int> midiProgram { 0 };
    Atomic<int> lastMidiProgram { -1 };
    Atomic<int> midiProgramsEnabled { 0 };
    Atomic<int> globalMidiPrograms { 0 };

    CriticalSection propertyLock;
    struct EnablementUpdater : public AsyncUpdater {
        EnablementUpdater (Processor& g) : graph (g) {}
        ~EnablementUpdater() {}
        void handleAsyncUpdate() override;
        Processor& graph;
    } enablement;

    struct MidiProgramLoader : public AsyncUpdater {
        MidiProgramLoader (Processor& n) : node (n) {}
        ~MidiProgramLoader() { cancelPendingUpdate(); }
        void handleAsyncUpdate() override;
        Processor& node;
    } midiProgramLoader;

    friend struct PortResetter;
    struct PortResetter : public AsyncUpdater {
        PortResetter (Processor& n) : node (n) {}
        ~PortResetter() { cancelPendingUpdate(); }
        void handleAsyncUpdate() override;
        Processor& node;
    } portResetter;

    struct MidiProgram {
        int program;
        String name;
        MemoryBlock state;
    };
    mutable OwnedArray<MidiProgram> midiPrograms;
    MidiProgram* getMidiProgram (int) const;

    void setParentGraph (GraphNode*);
    void prepare (double sampleRate, int blockSize, GraphNode*, bool willBeEnabled = false);
    void unprepare();
    void resetPorts();

    std::unique_ptr<Oversampler<float>> oversampler;
    int osPow = 0;
    float osLatency = 0.0f;
    dsp::Oversampling<float>* getOversamplingProcessor();

    ParameterPtr getOrCreateParameter (const PortDescription&);

    double delayCompMillis = 0.0;
    int delayCompSamples = 0;

    juce::AudioPlayHead* _playhead { nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Processor)
};

/** A convenient typedef for referring to a pointer to a node object. */
using ProcessorPtr = ReferenceCountedObjectPtr<Processor>;

} // namespace element
