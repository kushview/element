#ifndef EL_MIDI_SEQUENCE_PROCESSOR_H
#define EL_MIDI_SEQUENCE_PROCESSOR_H

#include "element/Juce.h"
#include "session/MidiClip.h"

namespace Element {

class AudioEngine;
    
class MidiSequenceProcessor : public Processor {
public:
    explicit MidiSequenceProcessor (AudioEngine&);
    ~MidiSequenceProcessor() { }
    
    const String getName() const { return "MIDI Sequencer"; }
    
    void prepareToPlay (double sampleRate, int estimatedBlockSize);
    void releaseResources();
    void processBlock (AudioSampleBuffer&, MidiBuffer&);
    
    const String getInputChannelName (int channelIndex) const { return String::empty; }
    const String getOutputChannelName (int channelIndex) const { return String::empty; }
    bool isInputChannelStereoPair (int index) const { return false; }
    bool isOutputChannelStereoPair (int index) const { return false; }
    bool silenceInProducesSilenceOut() const { return false; }
    double getTailLengthSeconds() const { return 0.0f; }
    
    bool acceptsMidi() const { return false; }
    bool producesMidi() const { return true; }
    
    bool hasEditor() const                          { return true; }
    AudioProcessorEditor* createEditor();
    
    int getNumParameters()                          { return 0; }
    const String getParameterName (int)             { return String::empty; }
    float getParameter (int)                        { return 0; }
    const String getParameterText (int)             { return String::empty; }
    void setParameter (int, float)                  { }
    
    int getNumPrograms()                            { return 0; }
    int getCurrentProgram()                         { return 0; }
    void setCurrentProgram (int)                    { }
    const String getProgramName (int)               { return String::empty; }
    void changeProgramName (int, const String&)     { }
    
    void getStateInformation (MemoryBlock&) { }
    void setStateInformation (const void* data, int sizeInBytes) { }
    
    void fillInPluginDescription (PluginDescription& d) const {
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
