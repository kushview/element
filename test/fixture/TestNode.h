
#pragma once
#include "engine/nodeobject.hpp"

namespace element {

class TestNode : public NodeObject
{
public:
    TestNode() : NodeObject (0) {}

    TestNode (int audioIns, int audioOuts, int midiIns, int midiOuts) 
        : NodeObject (0),
          numAudioIns (audioIns),
          numAudioOuts (audioOuts),
          numMidiIns (midiIns),
          numMidiOuts (midiOuts)
    {
        TestNode::refreshPorts();
    }

    void prepareToRender (double newSampleRate, int newBlockSize) override
    {
        setRenderDetails (newSampleRate, newBlockSize);
    }

    void releaseResources() override {}
    
    bool wantsMidiPipe() const override { return true; }

    void render (AudioSampleBuffer&, MidiPipe&) override { }

    void renderBypassed (AudioSampleBuffer&, MidiPipe&) override {}

    int getNumPrograms() const override { return 1; }
    int getCurrentProgram() const override { return 0; }
    const String getProgramName (int index) const override { return "program"; }
    void setCurrentProgram (int index) override {}

    void getState (MemoryBlock&) override {}
    void setState (const void*, int sizeInBytes) override {}
    
    void getPluginDescription (PluginDescription& desc) const override
    {
        desc.pluginFormatName = "Element";
        desc.fileOrIdentifier = "element.testNode";
        desc.manufacturerName = "Element";
    }

    virtual void refreshPorts() override
    {
        PortList newPorts;
        uint32 port = 0;
        for (int c = 0; c < numAudioIns; c++)
        {
            newPorts.add (PortType::Audio, port++, c,
                       String ("audio_in_") + String (c + 1),  
                       String ("In ") + String (c + 1), 
                       true);
        }

        for (int c = 0; c < numMidiIns; c++)
        {
            newPorts.add (PortType::Midi, port++, c,
                       String ("midi_in_") + String (c + 1),  
                       String ("MIDI In ") + String (c + 1), 
                       true);
        }

        for (int c = 0; c < numAudioOuts; c++)
        {
            newPorts.add (PortType::Audio, port++, c,
                       String ("audio_out_") + String (c + 1),  
                       String ("Out ") + String (c + 1), 
                       false);
        }

        for (int c = 0; c < numMidiOuts; c++)
        {
            newPorts.add (PortType::Midi, port++, c,
                       String ("midi_out_") + String (c + 1),  
                       String ("MIDI Out ") + String (c + 1), 
                       false);
        }

        setPorts (newPorts);
    }

protected:
    int numAudioIns     = 2,
        numAudioOuts    = 2,
        numMidiIns      = 1,
        numMidiOuts     = 1;

    bool prepared;
    double sampleRate;
    int bufferSize;

    void initialize() override {}
};

}
