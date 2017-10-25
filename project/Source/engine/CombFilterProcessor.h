
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
        }
        
        bufferSize = numSamples;
        if (bufferIndex >= bufferSize)
        {
            bufferIndex = 0;
        }
        
        clear();
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
        bufferSize = allocatedSize = 0;
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
        bufferIndex = 0;
        buffer.clear ((size_t) bufferSize);
    }
    
    void free()
    {
        bufferSize = 0;
        bufferIndex = 0;
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
    /*
         // Freeverb tunings
         25.306122448979592, 26.938775510204082,
         28.956916099773243, 30.748299319727891,
         32.244897959183673, 33.80952380952381,
         35.306122448979592, 36.666666666666667
     */
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
        desc.fileOrIdentifier   = stereo ? "element.comb.stereo" : "element.comb.mono";
        desc.descriptiveName    = stereo ? "Comb Filter (stereo)" : "Comb Filter (mono)";
        desc.numInputChannels   = stereo ? 2 : 1;
        desc.numOutputChannels  = stereo ? 2 : 1;
        desc.hasSharedContainer = false;
        desc.isInstrument       = false;
        desc.manufacturerName   = "Element";
        desc.pluginFormatName   = "Element";
        desc.version            = "1.0.0";
    }
    
    int spreadForChannel (const int c) const {
        return c * 28;
    }
    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override
    {
        lastLength = *length;
        for (int i = 0; i < 2; ++i)
            comb[i].setSize (spreadForChannel(i) + (*length * sampleRate * 0.001));
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

    
    
    
    
    
    
    
    
    
// MARK: Volume Processor

class VolumeProcessor : public BaseProcessor
{
private:
    const bool stereo;
    float lastVolume;
    float gain;
    float lastGain;
    AudioParameterFloat* volume = nullptr;
    
public:
    explicit VolumeProcessor (const double minDb, const double maxDb,
                              const bool _stereo = false)
        : BaseProcessor(),
          stereo (_stereo)
    {
        setPlayConfigDetails (stereo ? 2 : 1, stereo ? 2 : 1, 44100.0, 1024);
        addParameter (volume = new AudioParameterFloat ("volume", "Volume", minDb, maxDb, 0.f));
        lastVolume = *volume;
        gain = Decibels::decibelsToGain (lastVolume);
        lastGain = gain;
    }
    
    virtual ~VolumeProcessor()
    {
        volume = nullptr;
    }
    
    const String getName() const override { return "Volume"; }
    
    void fillInPluginDescription (PluginDescription& desc) const override
    {
        desc.name = getName();
        desc.fileOrIdentifier   = stereo ? "element.volume.stereo" : "element.volume.mono";
        desc.descriptiveName    = stereo ? "Volume (stereo)" : "Volume (mono)";
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
        setPlayConfigDetails (stereo ? 2 : 1, stereo ? 2 : 1,
                              sampleRate, maximumExpectedSamplesPerBlock);
    }
    
    void releaseResources() override
    {

    }
    
    void processBlock (AudioBuffer<float>& buffer, MidiBuffer&) override
    {
        if (lastVolume != (float) *volume) {
            gain = (float)*volume <= -30.f ? 0.f : Decibels::decibelsToGain ((float) *volume);
        }
        
        for (int c = jmin (2, buffer.getNumChannels()); --c >= 0;)
              buffer.applyGainRamp (c, 0, buffer.getNumSamples(), lastGain, gain);

        lastGain = gain;
        lastVolume = *volume;
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
        state.setProperty ("volume",  (float) *volume, 0);
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
                *volume = lastVolume = (float) state.getProperty ("volume",  (float) *volume);
                gain = lastGain = Decibels::decibelsToGain ((float) *volume);
            }
        }
    }
};

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    // MARK: All Pass Filter
    
class AllPassFilterProcessor : public BaseProcessor
{
private:
    const bool stereo;
    AudioParameterFloat* length = nullptr;

public:
    /*
     // Freeverb tunings

     */
    explicit AllPassFilterProcessor (const bool _stereo = false)
        : BaseProcessor(), stereo (_stereo)
    {
        setPlayConfigDetails (stereo ? 2 : 1, stereo ? 2 : 1, 44100.0, 1024);
        addParameter (length   = new AudioParameterFloat ("length",   "Buffer Length",  1.f, 500.f, 90.f));
        lastLength = *length;
    }
    
    virtual ~AllPassFilterProcessor()
    {
        length = nullptr;
    }
    
    const String getName() const override { return "AllPass Filter"; }
    
    void fillInPluginDescription (PluginDescription& desc) const override
    {
        desc.name = getName();
        desc.fileOrIdentifier   = stereo ? "element.allPass.stereo" : "element.allPass.mono";
        desc.descriptiveName    = stereo ? "AllPass Filter (stereo)" : "AllPass Filter (mono)";
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
            allPass[i].setSize (*length * sampleRate * 0.001);
        setPlayConfigDetails (stereo ? 2 : 1, stereo ? 2 : 1,
                              sampleRate, maximumExpectedSamplesPerBlock);
    }
    
    void releaseResources() override
    {
        for (int i = 0; i < 2; ++i)
            allPass[i].free();
    }
    
    void processBlock (AudioBuffer<float>& buffer, MidiBuffer&) override
    {
        if (lastLength != *length)
        {
            const int newSize = roundToIntAccurate (*length * getSampleRate() * 0.001);
            for (int i = 0; i < 2; ++i)
                allPass[i].setSize (newSize);
            lastLength = *length;
        }
        
        const int numChans = jmin (2, buffer.getNumChannels());
        const auto** input = buffer.getArrayOfReadPointers();
        auto** output = buffer.getArrayOfWritePointers();
        for (int c = 0; c < numChans; ++c)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                output[c][i] = allPass[c].process (input[c][i]);
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
                *length   = (float) state.getProperty ("length",   (float) *length);
        }
    }
    
private:
    AllPassFilter allPass[2];
    float lastLength;
};

    

    
    
    
    // MARK: Wet/Dry Processor
    
    class WetDryProcessor : public BaseProcessor
    {
    private:
        AudioParameterFloat* wetLevel = nullptr;
        AudioParameterFloat* dryLevel = nullptr;
        float lastWetLevel = 0.33f;
        float lastDryLevel = 0.40f;
        
    public:
        explicit WetDryProcessor()
            : BaseProcessor()
        {
            setPlayConfigDetails (4, 2, 44100.0, 1024);
            addParameter (wetLevel = new AudioParameterFloat ("wetLevel",   "Wet Level",  0.f, 1.f, 0.33f));
            addParameter (dryLevel = new AudioParameterFloat ("dryLevel",   "Dry Level",  0.f, 1.f, 0.40f));
        }
        
        virtual ~WetDryProcessor()
        {
            wetLevel = dryLevel = nullptr;
        }
        
        const String getName() const override { return "Wet/Dry"; }
        
        void fillInPluginDescription (PluginDescription& desc) const override
        {
            desc.name = getName();
            desc.fileOrIdentifier   = "element.wetDry";
            desc.version            = "1.0.0";
            desc.descriptiveName    = "Combines stereo wet/dry signals in to a single stereo output.";
            desc.numInputChannels   = 4;
            desc.numOutputChannels  = 2;
            desc.hasSharedContainer = false;
            desc.isInstrument       = false;
            desc.manufacturerName   = "Element";
            desc.pluginFormatName   = "Element";
        }
        
        void setLevels (const float newWet, const float newDry)
        {
            const float wetScaleFactor = 3.0f;
            const float dryScaleFactor = 2.0f;
            const float width          = 1.f;
            const float wet = newWet * wetScaleFactor;
            
            dryGain.setValue (newDry * dryScaleFactor);
            wetGain1.setValue (0.5f * wet * (1.0f + width));
            wetGain2.setValue (0.5f * wet * (1.0f - width));
        }
        
        void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override
        {
            setPlayConfigDetails (4, 2, sampleRate, maximumExpectedSamplesPerBlock);
            const double smoothTime = 0.01;
            dryGain .reset (sampleRate, smoothTime);
            wetGain1.reset (sampleRate, smoothTime);
            wetGain2.reset (sampleRate, smoothTime);
            lastWetLevel = (float) *wetLevel;
            lastDryLevel = (float) *dryLevel;
        }
        
        void releaseResources() override { }
        
        void processBlock (AudioBuffer<float>& buffer, MidiBuffer&) override
        {
            if (lastWetLevel != (float)*wetLevel || lastDryLevel != (float)*dryLevel)
                setLevels (*wetLevel, *dryLevel);
            
            if (buffer.getNumChannels() >= 4)
            {
                const int numSamples = buffer.getNumSamples();
                const auto** input  = buffer.getArrayOfReadPointers();
                auto** output = buffer.getArrayOfWritePointers();
                
                for (int i = 0; i < numSamples; ++i)
                {
                    const float dry  = dryGain.getNextValue();
                    const float wet1 = wetGain1.getNextValue();
                    const float wet2 = wetGain2.getNextValue();
                    
                    output[0][i] = input[0][i] * wet1 + input[1][i] * wet2 + input[2][i] * dry;
                    output[1][i] = input[1][i] * wet1 + input[0][i] * wet2 + input[3][i] * dry;
                }
            }
            else
            {
                DBG("CHans: " << buffer.getNumChannels());
            }
            
            lastWetLevel = *wetLevel;
            lastDryLevel = *dryLevel;
        }
        
        AudioProcessorEditor* createEditor() override
        {
            auto* ed = new GenericAudioProcessorEditor (this);
            ed->resized();
            return ed;
        }
        
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
            state.setProperty ("wetLevel",   (float) *wetLevel, 0);
            state.setProperty ("dryLevel",   (float) *dryLevel, 0);
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
                    *wetLevel = (float) state.getProperty ("wetLevel", (float) *wetLevel);
                    *dryLevel = (float) state.getProperty ("dryLevel", (float) *dryLevel);
                }
            }
        }
        
    private:
        LinearSmoothedValue<float> dryGain, wetGain1, wetGain2;
    };

    


}
