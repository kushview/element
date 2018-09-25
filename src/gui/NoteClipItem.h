/*
    NoteClipItem.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "session/Note.h"

namespace Element {
class NoteClipItem :  public TimelineClip
{
public:

    typedef std::pair<MidiMessage, MidiMessage> MsgPair;

    NoteClipItem (TimelineComponent& owner, const Note& m)
        : TimelineClip (owner),
          model (m)
    {
        jassert (model.isValid());
        jassert (isPositiveAndBelow (model.keyId(), 128));
        jassert (model.channel() >= 1 && model.channel() <= 16);
        colour.addColour (0.0, Colours::lightsalmon);
        colour.addColour (1.0, Colours::red);
        trackRequested (127 - model.keyId());
    }

    virtual ~NoteClipItem() { }

    inline void setModel (const Note& n) {
        model = n;
    }

    inline void reset() {
        Note null (Note::make (ValueTree()));
        setModel (null);
    }

    inline void
    getTime (Range<double> &time) const
    {
        time.setStart (model.tickStart());
        time.setEnd (model.tickEnd());
    }

    inline void
    setTime (const Range<double> &time)
    {
        model.move (deltas, (time.getStart()));
        model.resize (deltas, (time.getLength()));
        model.applyEdits (deltas);
    }

    inline TimeUnit getTimeUnit() const { return TimeUnit::Ticks; }

    inline void
    paint (Graphics &g)
    {
        if (! isSelected())
            g.setColour (fillColor (model.velocity()));
        else
            g.setColour (Colours::aqua);

        g.fillAll();

        g.setColour (Colours::black);
        g.drawRect (getLocalBounds(), 1);

        if (model.isValid())
            g.drawFittedText (String (model.keyId()),
                              getLocalBounds(), Justification::centred, 1);
    }

    inline int32 trackIndex() const { return 127 - model.keyId(); }

    inline int32
    trackRequested (const int32 track)
    {
        if (! isPositiveAndBelow (track, 128))
            return trackIndex();

        model.changeKeyId (deltas, 127 - track);
        model.applyEdits (deltas);
        return trackIndex();
    }

    const int channel() const { return model.channel(); }
    const int keyId() const { return model.keyId(); }
    const float velocity() const { return model.velocity(); }

    MsgPair makeMidi() const
    {
        MidiMessage non (MidiMessage::noteOn (model.channel(),
                                              model.keyId(),
                                              (float) model.velocity()));
        MidiMessage noff (MidiMessage::noteOff (model.channel(), model.keyId(), 0.0f));
        non.setTimeStamp (model.tickStart());
        noff.setTimeStamp (model.tickEnd());
        return std::make_pair (non, noff);
    }

    const Note& note() const { return model; }

private:
    Note model;
    Note::EditDeltas deltas;

    Range<double> beatTime;
    int midiChannel, midiNote;
    float noteVelocity;

    ColourGradient colour;

    inline Colour fillColor (float velocity)
    {
        return colour.getColourAtPosition (velocity);
    }
};
}
