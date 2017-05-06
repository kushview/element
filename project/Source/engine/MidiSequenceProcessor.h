#ifndef EL_MIDI_SEQUENCE_PROCESSOR_H
#define EL_MIDI_SEQUENCE_PROCESSOR_H

#include "ElementApp.h"
#include "session/MidiClip.h"

namespace Element {

class AudioEngine;
class ClipSource;

class MidiSequenceProcessor : public Processor {
public:
    explicit MidiSequenceProcessor (AudioEngine&);
    ~MidiSequenceProcessor() { }
    
    const String getName() const override { return "MIDI Sequencer"; }
    
    void prepareToPlay (double sampleRate, int estimatedBlockSize) override;
    void releaseResources() override;
    void processBlock (AudioSampleBuffer&, MidiBuffer&) override;
    
    const String getInputChannelName (int) const override { return String::empty; }
    const String getOutputChannelName (int) const override { return String::empty; }
    bool isInputChannelStereoPair (int) const override { return false; }
    bool isOutputChannelStereoPair (int) const override { return false; }
    bool silenceInProducesSilenceOut() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0f; }
    
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return true; }
    
    bool hasEditor() const override                 { return true; }
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
        d.fileOrIdentifier = "element.midiSequence";
        d.name = getName();
        d.pluginFormatName = "Internal";
        d.manufacturerName = "Element";
        d.version = "1.0";
    }
    
private:
    AudioEngine& engine;
    MidiSequencePlayer player;
    MidiMessageSequence seq;
    MidiClip clip;
    ClipSource* source;
};
    
}

#endif  // EL_MIDI_SEQUENCE_PROCESSOR_H
