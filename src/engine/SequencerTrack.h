/*
    SequencerTrack.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
*/

#pragma once

class Sequencer;

class SequencerTrack :  public Processor
{
public:
    SequencerTrack (Sequencer& o, const TrackModel& t);
    virtual ~SequencerTrack();

    const int32 index() const { return trackIndex; }

    // AudioPluginInstance
    inline const String getName() const { return state.hasProperty("name") ?
                                                 state.getProperty("name") : "Sequencer Track"; }

    void prepareToPlay (double sampleRate, int blockSize);
    void releaseResources();
    void processBlock (AudioSampleBuffer &buffer, MidiBuffer &midi);
    void processBlockBypassed (AudioSampleBuffer&, MidiBuffer&);

    virtual const String getInputChannelName (int /*channelIndex*/) const { return String(); }
    virtual const String getOutputChannelName (int /*channelIndex*/) const { return String(); }
    virtual bool isInputChannelStereoPair (int /*index*/) const { return false; }
    virtual bool isOutputChannelStereoPair (int /*index*/) const { return false; }
    virtual void numChannelsChanged() { }

    virtual bool silenceInProducesSilenceOut() const { return false; }
    virtual double getTailLengthSeconds() const { return 0.0f; }

    virtual bool acceptsMidi() const { return false; }
    virtual bool producesMidi() const { return false; }

    virtual AudioProcessorEditor* createEditor() { return nullptr; }
    virtual bool hasEditor() const { return false; }

    virtual int getNumParameters() { return 0; }
    const String getParameterName (int /*index*/) { return String(); }
    float getParameter (int /*index*/) { return 0.0f; }
    const String getParameterText (int /*index*/) { return String(); }
    virtual String getParameterName (int /*parameterIndex*/, int /*maximumStringLength*/) { return String(); }
    virtual String getParameterText (int /*parameterIndex*/, int /*maximumStringLength*/) { return String(); }
    virtual void setParameter (int, float) { }

    virtual int getNumPrograms() { return 0; }
    virtual int getCurrentProgram() { return 0; }
    virtual void setCurrentProgram (int /*index*/) { }
    virtual const String getProgramName (int /*index*/) { return String(); }
    virtual void changeProgramName (int /*index*/, const String& /*newName*/) { }

    virtual void getStateInformation (MemoryBlock& /*destData*/) { }
    virtual void setStateInformation (const void* /*data*/, int /*sizeInBytes*/) { }

    virtual void fillInPluginDescription (PluginDescription& /*description*/) const { }

    inline ClipSource* firstClip()  const { return bin.first(); }
    inline ClipSource* lastClip()   const { return bin.last(); }

protected:

    Sequencer& sequencer;
    ClipSource* cursorClip() const;

private:

    ScopedPointer<TrackModel> track;
    ValueTree state;

    Value armed, muted, soloed, volume;
    int32 trackIndex;

    ClipBin bin;

    friend class Sequencer;

#if 0
    // see Sequencer for current implementation of value callbacks
    friend class ValueTree;
    void valueTreePropertyChanged (ValueTree& tree, const Identifier& property);
    void valueTreeChildAdded (ValueTree& parent, ValueTree& childWhichHasBeenAdded);
    void valueTreeChildRemoved (ValueTree& parent, ValueTree& childWhichHasBeenRemoved);
    void valueTreeChildOrderChanged (ValueTree& parent);
    void valueTreeParentChanged (ValueTree& tree);
#endif

};
