
#pragma once

#include "engine/BaseProcessor.h"

namespace Element {

    class ReverbProcessor : public BaseProcessor
    {
    public:
        explicit ReverbProcessor()
            : BaseProcessor()
        {
            setPlayConfigDetails (2, 2, 44100.0, 1024);
            addParameter (new AudioParameterFloat (Tags::volume.toString(), "Volume", minDb, maxDb, 0.f));
        }

        virtual ~ReverbProcessor()
        {

        }

        const String getName() const override { return "eVerb"; }
        
        void fillInPluginDescription (PluginDescription& desc) const override
        {
            desc.name = getName();
            desc.fileOrIdentifier   = "element.reverb";
            desc.descriptiveName    = "Simple Reverb node"
            desc.numInputChannels   = 2;
            desc.numOutputChannels  = 2;
            desc.hasSharedContainer = false;
            desc.isInstrument       = false;
            desc.manufacturerName   = "Element";
            desc.pluginFormatName   = "Element";
            desc.version            = "1.0.0";
        }
        
        void prepareToPlay (double sampleRate, int maxBlockSize) override { setPlayConfigDetails (2, 2, sampleRate, maxBlockSize); }
        void releaseResources() override { }
        void processBlock (AudioBuffer<float>& buffer, MidiBuffer&) override { }
        
        AudioProcessorEditor* createEditor() override   { return new GenericAudioProcessorEditor (this); }
        bool hasEditor() const override                 { return true; }
        
        double getTailLengthSeconds() const override    { return 0.0; };
        bool acceptsMidi() const override               { return false; }
        bool producesMidi() const override              { return false; }
        
        int getNumPrograms() override                                      { return 1; };
        int getCurrentProgram() override                                   { return 1; };
        void setCurrentProgram (int index) override                        { ignoreUnused (index); };
        const String getProgramName (int index) override                   { ignoreUnused (index); return "Default"; }
        void changeProgramName (int index, const String& newName) override { ignoreUnused (index, newName); }
        
        void getStateInformation (juce::MemoryBlock& destData) override
        {
            ValueTree state (Tags::state);
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
                    
                }
            }
        }
    };
}
