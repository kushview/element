/*
    Sequencer.cpp - This file is part of Element

    Copyright (C) 2013 Kushview, LLC  All rights reserved.
      * Michael Fisher <mfisher@kushview.net>
*/

#if JUCE_COMPLETION
#include "modules/element_engines/element_engines.h"
#endif

static Sequencer::NodePtr
trackNode (const TrackModel& t) {
    return dynamic_cast<GraphNode*> (
        t.state().getProperty ("node", var::null).getObject());
}

static SequencerTrack* trackProcessor (const TrackModel& t)
{
    if (Sequencer::NodePtr ptr = trackNode (t))
        return ptr->processor<SequencerTrack>();
    return nullptr;
}

Sequencer::Sequencer (ClipFactory& e)
    : engine (e)
{
    cursor = new SequenceCursor (*this, 0);
    trackRefs.ensureStorageAllocated (32);
    setPlayConfigDetails (0, 4, 44100.f, 1024);
    addNode (new GraphProcessor::AudioGraphIOProcessor (
                 GraphProcessor::AudioGraphIOProcessor::audioOutputNode), 200);
    state.addListener (this);
}

Sequencer::~Sequencer()
{
    state.removeListener (this);

    cursor = nullptr;
    this->trackRefs.clear();
    clear();
    state = ValueTree::invalid;
}

bool Sequencer::tracksArePresent() const
{
	for (int i = 0; i < trackRefs.size(); ++i)
		if (nullptr == trackRefs.getUnchecked(i))
			return false;
    return true;
}

bool Sequencer::addTrack (const TrackModel &t)
{
    NodePtr node = createTrackFor (t);
    return node != nullptr;
}

Sequencer::NodePtr
Sequencer::createTrackFor (const TrackModel& t)
{
    if (NodePtr node = getNodeFor (t))
        return node;

    NodePtr newNode = addNode (new SequencerTrack (*this, t));

    if (newNode != nullptr)
    {
        connectChannels (PortType::Audio, newNode->nodeId, 0, 200, 0);
        connectChannels (PortType::Audio, newNode->nodeId, 1, 200, 1);

        SequencerTrack* proc = newNode->processor<SequencerTrack>();

        while (t.index() >= trackRefs.size())
            trackRefs.add (nullptr);

        trackRefs.set (t.index(), proc);
        proc->trackIndex = t.index();
        cursor->addTrack();

        t.state().setProperty ("node", newNode.get(), nullptr);

        jassert (numTracks() == trackRefs.size());
        jassert (trackProcessor (t) != nullptr);
    }

    return newNode;
}

void
Sequencer::assignTrack (int32 index, TrackModel &track)
{
    NodePtr node = getNodeFor (track);

    if (! node)
        node = createTrackFor (track);

    if (node)
    {
        track.state().setProperty ("node", node.get(), nullptr);
        SequencerTrack* st = node->processor<SequencerTrack>();

        if (st->index() != track.index()) {
            jassertfalse;
            //not sure what do do yet
        }

        // clear and add all clips for this track
        st->bin.clear();
        ValueTree parent = track.state();
        for (int32 t = 0; t < track.state().getNumChildren(); t++)
        {
            ValueTree child (parent.getChild (t));
            valueTreeChildAdded (parent, child);
        }
    }
    else
    {
        track.state().removeProperty ("node", nullptr);
    }
}


Sequencer::NodePtr
Sequencer::getNodeFor (const ValueTree& state)
{
    NodePtr invalid = nullptr;
    NodePtr ptr = dynamic_cast<GraphNode*> (state.getProperty ("node", var::null).getObject());

    if (! ptr)
        return invalid;

    for (int i = getNumNodes(); --i >= 0;)
        if (ptr == getNode(i))
            return ptr;

    return invalid;
}

Sequencer::NodePtr
Sequencer::getNodeFor (const TrackModel& track)
{
    ValueTree state (track.state());
    return getNodeFor (state);
}

void
Sequencer::setModel (const SequenceModel& m)
{
    state = m.node();
}

SequenceModel
Sequencer::model() const
{
    SequenceModel m (state);
    return m;
}

int32 Sequencer::numTracks() const { return cursor != nullptr ? cursor->numTracks() : 0; }

void
Sequencer::prepareToPlay (double sampleRate, int blockSize)
{
    if (! cursor)
        cursor = new SequenceCursor (*this, 0, DataType::Audio);

    GraphProcessor::prepareToPlay (sampleRate, blockSize);
}

void
Sequencer::releaseResources()
{
    GraphProcessor::releaseResources();
}

void
Sequencer::preRenderNodes()
{
    getPlayHead()->getCurrentPosition (playPos);
}

void
Sequencer::postRenderNodes()
{
    cursor->seek (playPos.timeInSamples + getBlockSize());
}

bool
Sequencer::removeTrack (const TrackModel& track)
{
    if (NodePtr node = getNodeFor (track))
    {
        if (removeNode (node->nodeId))
        {
            SequencerTrack* st = node->processor<SequencerTrack>();
            assert (st != nullptr);
            assert (trackRefs.contains (st));

            const int32 index = trackRefs.indexOf (st);
            assert (index == st->index());

            {
                ScopedLock sl (getCallbackLock());

                cursor->removeTrack (st);
                trackRefs.remove (index);

                for (int i = index; i < trackRefs.size(); ++i)
                    trackRefs.getUnchecked(i)->trackIndex = i;
            }

            track.state().removeProperty ("node", nullptr);

            assert (numTracks() == trackRefs.size());
            return true;
        }
    }

    assert (numTracks() == trackRefs.size());
    return false;
}

ClipSource*
Sequencer::trackClip (int32 track) const
{
    return cursor->clip (track);
}

void
Sequencer::updateTrack (int32 index)
{
    if (isPositiveAndBelow (index, trackRefs.size()))
        cursor->updateTrack (trackRefs.getUnchecked (index));
}


void Sequencer::valueTreePropertyChanged (ValueTree& tree, const Identifier& property) { }

void
Sequencer::valueTreeChildAdded (ValueTree& parent, ValueTree& child)
{
    if (parent != state && parent.getParent() != state)
        return;

    if (child.hasType (Slugs::track))
    {
        TrackModel track (child);
        DBG ("Track Model Added: " + String (track.index()));
        if (track.isValid())
            addTrack (track);
    }
    else if (child.hasType (Slugs::clip))
    {
        if (SequencerTrack* track = trackProcessor (TrackModel (parent)))
        {
            ClipModel model (child);
            if (ClipSource* clip = engine.createSource (model))
            {
                clip->prepareToPlay (getBlockSize(), getSampleRate());
                track->bin.append (clip);
                assert (track == trackRefs [track->index()]);
                updateTrack (track->index());
            }
            else
            {
                DBG ("ClipFactory failed to create clip");
            }
        }
        else
        {
            DBG (" couldn't extract a Track processor from the model");
        }
    }
}

void Sequencer::valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int)
{
    if (parent != state && parent.getParent() != state)
        return;

    if (child.hasType (Slugs::track))
    {
        TrackModel track (child);
        if (track.isValid())
            removeTrack (track);
    }

#if 0
    if (! child.hasType (Slugs::clip))
        return;

    ClipSource* found = nullptr;

    for (ClipSource* clip : bin) {
        if (clip->clipData == child)
            { found = clip; break; }
    }

    if (found)
    {
        bin.remove (found);
        sequencer.updateTrack (trackIndex);
        std::clog << "finally removed Source Clip\n";
    }
#endif
}

void Sequencer::valueTreeChildOrderChanged (ValueTree& parent, int, int)
{

}

void Sequencer::valueTreeParentChanged (ValueTree& tree)
{

}
