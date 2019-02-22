/*
    Sequencer.h - This file is part of Element

    Copyright (C) 2013 Kushview, LLC  All rights reserved.
      * Michael Fisher <mfisher@kushview.net>
*/

#ifndef ELEMENT_SEQUENCER_H
#define ELEMENT_SEQUENCER_H

class ClipFactory;
class ClipSource;
class SequenceCursor;
class SequenceModel;
class SequencerTrack;
class TrackModel;

/** The engine-side main sequencer implementation */
class Sequencer :  public GraphProcessor,
                   public ValueTree::Listener
{
public:
    typedef AudioPlayHead::CurrentPositionInfo Position;
    typedef GraphNodePtr NodePtr;
    typedef ReferenceCountedArray<GraphNode> NodeArray;

    /** Create a new sequencer graph
        Sequencer needs the Audio engine for managing clip data as
        well as media management globals */
    Sequencer (ClipFactory& c);
    ~Sequencer ();

    int32 numTracks() const;

    bool addTrack (const TrackModel& t);
    void assignTrack (int32 index, TrackModel& t);
    bool removeTrack (const TrackModel& t);

    bool tracksArePresent() const;
    ClipSource* trackClip (int32 track) const;
    const Array<SequencerTrack*>& tracks() const { return trackRefs; }

    const String getName() const override { return "Sequencer"; }
    void prepareToPlay (double sampleRate, int estimatedBlockSize) override;
    void releaseResources() override;

    const Position& position() const { return playPos; }

    void setModel (const SequenceModel& m);
    SequenceModel model() const;

    void updateTrack (int32 index);

private:

    ValueTree state;

    ClipFactory& engine;
    ScopedPointer<SequenceCursor> cursor;
    Array<SequencerTrack*> trackRefs;
    Position playPos;

    NodePtr createTrackFor (const TrackModel& track);
    NodePtr getNodeFor (const TrackModel& t);
    NodePtr getNodeFor (const ValueTree& vt);

    void preRenderNodes() override;
    void postRenderNodes() override;

    friend class ValueTree;
    void valueTreePropertyChanged (ValueTree& tree, const Identifier& property) override;
    void valueTreeChildAdded (ValueTree& parent, ValueTree& child) override;
    void valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int) override;
    void valueTreeChildOrderChanged (ValueTree& parent, int, int) override;
    void valueTreeParentChanged (ValueTree& tree) override;


    JUCE_LEAK_DETECTOR (Sequencer)
};

#endif // ELEMENT_SEQUENCER_H
