#pragma once

#include "ElementApp.h"
#include "session/MidiClip.h"

namespace Element {

class AudioEngine;
class ClipSource;

class MidiSequenceProcessor : public Processor
{
public:
    explicit MidiSequenceProcessor();
    ~MidiSequenceProcessor() { }
    
    const String getName() const override { return "MIDI Sequencer"; }
    
    void prepareToPlay (double sampleRate, int estimatedBlockSize) override;
    void releaseResources() override;
    void processBlock (AudioSampleBuffer&, MidiBuffer&) override;

    double getTailLengthSeconds() const override { return 0.0f; }
    
    bool isMidiEffect() const { return true; }
    bool acceptsMidi()  const override { return true; }
    bool producesMidi() const override { return true; }

    bool hasEditor() const override { return true; }
    AudioProcessorEditor* createEditor() override;
    
    int getNumParameters() override                       { return 0; }
    const String getParameterName (int) override          { return String::empty; }
    float getParameter (int) override                     { return 0; }
    const String getParameterText (int) override          { return String::empty; }
    void setParameter (int, float) override               { }
    
    int getNumPrograms() override                         { return 0; }
    int getCurrentProgram() override                      { return 0; }
    void setCurrentProgram (int) override                 { }
    const String getProgramName (int) override            { return String::empty; }
    void changeProgramName (int, const String&) override  { }
    
    void getStateInformation (MemoryBlock&) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    void fillInPluginDescription (PluginDescription& d) const override
    {
        d.fileOrIdentifier = "element.midiSequencer";
        d.name = getName();
        d.pluginFormatName = "Element";
        d.manufacturerName = "Kushview";
        d.version = "1.0.0";
    }
    
    const String getInputChannelName (int) const override { return String::empty; }
    const String getOutputChannelName (int) const override { return String::empty; }
    bool isInputChannelStereoPair (int) const override { return false; }
    bool isOutputChannelStereoPair (int) const override { return false; }
    bool silenceInProducesSilenceOut() const override { return false; }

private:
    MidiSequencePlayer player;
    MidiMessageSequence seq;
    MidiClip clip;
    ClipSource* source;
};
    
}
