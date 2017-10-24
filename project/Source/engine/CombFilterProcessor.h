
#pragma once

#include "ElementApp.h"

namespace Element {

class BaseProcessor : public AudioPluginInstance
{
public:
    BaseProcessor() { }
    virtual ~BaseProcessor() { }
    
#if 0
    // Audio Processor Template
    virtual const String getName() const = 0;
    virtual StringArray getAlternateDisplayNames() const;
    virtual void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) = 0;
    virtual void releaseResources() = 0;
    
    virtual void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) = 0;
    virtual void processBlock (AudioBuffer<double>& buffer, idiBuffer& midiMessages);
    
    virtual void processBlockBypassed (AudioBuffer<float>& buffer, MidiBuffer& midiMessages);
    virtual void processBlockBypassed (AudioBuffer<double>& buffer, MidiBuffer& midiMessages);

    virtual bool canAddBus (bool isInput) const                     { ignoreUnused (isInput); return false; }
    virtual bool canRemoveBus (bool isInput) const                  { ignoreUnused (isInput); return false; }
    virtual bool supportsDoublePrecisionProcessing() const;
    
    virtual double getTailLengthSeconds() const = 0;
    virtual bool acceptsMidi() const = 0;
    virtual bool producesMidi() const = 0;
    virtual bool supportsMPE() const                            { return false; }
    virtual bool isMidiEffect() const                           { return false; }
    virtual void reset();
    virtual void setNonRealtime (bool isNonRealtime) noexcept;
    
    virtual AudioProcessorEditor* createEditor() = 0;
    virtual bool hasEditor() const = 0;
    
    virtual int getNumPrograms()        { return 1; };
    virtual int getCurrentProgram()     { return 1; };
    virtual void setCurrentProgram (int index) { ignoreUnused (index); };
    virtual const String getProgramName (int index) { ignoreUnused (index); }
    virtual void changeProgramName (int index, const String& newName) { ignoreUnused (index, newName); }
    virtual void getStateInformation (juce::MemoryBlock& destData) = 0;
    virtual void getCurrentProgramStateInformation (juce::MemoryBlock& destData);
   
    virtual void setStateInformation (const void* data, int sizeInBytes) = 0;
    virtual void setCurrentProgramStateInformation (const void* data, int sizeInBytes);
    
    virtual void numChannelsChanged();
    virtual void numBusesChanged();
    virtual void processorLayoutsChanged();
    
    virtual void addListener (AudioProcessorListener* newListener);
    virtual void removeListener (AudioProcessorListener* listenerToRemove);
    virtual void setPlayHead (AudioPlayHead* newPlayHead);
    
    virtual void updateTrackProperties (const TrackProperties& properties);
    
    virtual void fillInPluginDescription (PluginDescription& description);
    
protected:
    virtual bool isBusesLayoutSupported (const BusesLayout&) const          { return true; }
    virtual bool canApplyBusesLayout (const BusesLayout& layouts) const     { return isBusesLayoutSupported (layouts); }
    virtual bool canApplyBusCountChange (bool isInput, bool isAddingBuses, BusProperties& outNewBusProperties);
#endif

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BaseProcessor)
};


class CombFilter
{
public:
    CombFilter() noexcept
        : bufferSize(0), bufferIndex(0), allocatedSize(0), last(0) { }
    
    void setSize (const int numSamples)
    {
        if (numSamples == bufferSize)
            return;
        
        const int proposedSize = nextPowerOfTwo (numSamples);
        if (proposedSize > allocatedSize)
        {
            buffer.realloc (static_cast<size_t> ( proposedSize));
            allocatedSize = proposedSize;
            clear();
        }
        
        bufferSize = numSamples;
        if (bufferIndex >= bufferSize) {
            bufferIndex = 0;
        }
    }
    
    void clear() noexcept
    {
        last = 0;
        bufferIndex = 0;
        buffer.clear ((size_t) bufferSize);
    }
    
    void free()
    {
        last = 0;
        bufferSize = 0;
        buffer.free();
    }
    
    float process (const float input, const float damp, const float feedbackLevel) noexcept
    {
        const float output = buffer [bufferIndex];
        last = (output * (1.0f - damp)) + (last * damp);
        JUCE_UNDENORMALISE (last);
        
        float temp = input + (last * feedbackLevel);
        JUCE_UNDENORMALISE (temp);
        buffer [bufferIndex] = temp;
        bufferIndex = (bufferIndex + 1) % bufferSize;
        return output;
    }
    
private:
    HeapBlock<float> buffer;
    int bufferSize, bufferIndex, allocatedSize;
    float last;
    
    JUCE_DECLARE_NON_COPYABLE (CombFilter)
};

class AllPassFilter
{
public:
    AllPassFilter() noexcept : bufferSize (0), bufferIndex (0) {}
    
    void setSize (const int size)
    {
        if (size != bufferSize)
        {
            bufferIndex = 0;
            buffer.malloc ((size_t) size);
            bufferSize = size;
        }
        
        clear();
    }
    
    void clear() noexcept
    {
        buffer.clear ((size_t) bufferSize);
    }
    
    void free()
    {
        buffer.free();
    }
    
    float process (const float input) noexcept
    {
        const float bufferedValue = buffer [bufferIndex];
        float temp = input + (bufferedValue * 0.5f);
        JUCE_UNDENORMALISE (temp);
        buffer [bufferIndex] = temp;
        bufferIndex = (bufferIndex + 1) % bufferSize;
        return bufferedValue - input;
    }
    
private:
    HeapBlock<float> buffer;
    int bufferSize, bufferIndex;
    
    JUCE_DECLARE_NON_COPYABLE (AllPassFilter)
};

class CombFilterProcessor : public BaseProcessor
{
private:
    const bool stereo;
    AudioParameterFloat* length   = nullptr;
    AudioParameterFloat* damping  = nullptr;
    AudioParameterFloat* feedback = nullptr;
    
public:
    explicit CombFilterProcessor (const bool _stereo = false)
        : BaseProcessor(),
          stereo (_stereo)
    {
        setPlayConfigDetails (stereo ? 2 : 1, stereo ? 2 : 1, 44100.0, 1024);
        addParameter (length   = new AudioParameterFloat ("length",   "Buffer Length",  1.f, 500.f, 90.f));
        lastLength = *length;
        addParameter (damping  = new AudioParameterFloat ("damping",  "Damping",        0.f, 1.f, 0.f));
        addParameter (feedback = new AudioParameterFloat ("feedback", "Feedback Level", 0.f, 1.f, 0.5f));
    }
    
    virtual ~CombFilterProcessor()
    {
        length = nullptr;
    }
    
    const String getName() const override { return "Comb Filter"; }

    void fillInPluginDescription (PluginDescription& desc) const override
    {
        desc.name = getName();
        desc.fileOrIdentifier   = stereo ? "element.combFilter.stereo" : "element.combFilter.mono";
        desc.descriptiveName    = stereo ? "Comb Filter (stereo)" : "Comb Filter (mono)";
        desc.numInputChannels   = stereo ? 2 : 1;
        desc.numOutputChannels  = stereo ? 2 : 1;
        desc.hasSharedContainer = false;
        desc.isInstrument       = false;
        desc.manufacturerName   = "Element";
        desc.pluginFormatName   = "Element";
        desc.version            = "1.0.0";
    }
    
    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override
    {
        lastLength = *length;
        for (int i = 0; i < 2; ++i)
            comb[i].setSize (*length * sampleRate * 0.001);
        setPlayConfigDetails (stereo ? 2 : 1, stereo ? 2 : 1,
                              sampleRate, maximumExpectedSamplesPerBlock);
    }
    
    void releaseResources() override
    {
        for (int i = 0; i < 2; ++i)
            comb[i].free();
    }
    
    void processBlock (AudioBuffer<float>& buffer, MidiBuffer&) override
    {
        if (lastLength != *length)
        {
            const int newSize = roundToIntAccurate (*length * getSampleRate() * 0.001);
            for (int i = 0; i < 2; ++i)
                comb[i].setSize (newSize);
            lastLength = *length;
        }
        
        const int numChans = jmin (2, buffer.getNumChannels());
        const auto** input = buffer.getArrayOfReadPointers();
        auto** output = buffer.getArrayOfWritePointers();
        for (int c = 0; c < numChans; ++c)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                output[c][i] = comb[c].process (input[c][i], *damping, *feedback);
    }

    AudioProcessorEditor* createEditor() override   { return new GenericAudioProcessorEditor (this); }
    bool hasEditor() const override                 { return true; }
    
    double getTailLengthSeconds() const override    { return 0.0; };
    bool acceptsMidi() const override               { return false; }
    bool producesMidi() const override              { return false; }
    
    int getNumPrograms() override                                      { return 1; };
    int getCurrentProgram() override                                   { return 1; };
    void setCurrentProgram (int index) override                        { ignoreUnused (index); };
    const String getProgramName (int index) override                   { ignoreUnused (index); return "Parameter"; }
    void changeProgramName (int index, const String& newName) override { ignoreUnused (index, newName); }
    
    void getStateInformation (juce::MemoryBlock& destData) override
    {
        ValueTree state (Tags::state);
        state.setProperty ("damping",  (float) *damping, 0);
        state.setProperty ("feedback", (float) *feedback, 0);
        state.setProperty ("length",   (float) *length, 0);
        if (ScopedPointer<XmlElement> e = state.createXml())
            AudioProcessor::copyXmlToBinary (*e, destData);
    }
    
    void setStateInformation (const void* data, int sizeInBytes) override
    {
        if (ScopedPointer<XmlElement> e = AudioProcessor::getXmlFromBinary (data, sizeInBytes))
        {
            auto state = ValueTree::fromXml (*e);
            if (state.isValid())
            {
                *damping  = (float) state.getProperty ("damping",  (float) *damping);
                *feedback = (float) state.getProperty ("feedback", (float) *feedback);
                *length   = (float) state.getProperty ("length",   (float) *length);
            }
        }
    }

private:
    CombFilter comb[2];
    float lastLength;
};

}
