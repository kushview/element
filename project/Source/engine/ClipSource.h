/*
    ClipSource.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef ELEMENT_CLIP_SOURCE_H
#define ELEMENT_CLIP_SOURCE_H

#include "element/Juce.h"

namespace Element {

class ClipFactory;
class ClipModel;

class ClipData
{
public:
    ClipData() { }
    virtual ~ClipData() { }

    virtual void prepare (double /*rate*/, int32 /*blockSize*/) { }
    virtual void unprepare () { }

    MidiMessageSequence midi;

protected:
    virtual void clipModelChanged (const ClipModel& model) { }
    
private:
    friend class ClipFactory;
    friend class ClipSource;
    int64 hash;
};

class ClipSource :  public LinkedList<ClipSource>::Link,
                    public PositionableAudioSource,
                    public ValueListener
{
public:
    typedef AudioPlayHead::CurrentPositionInfo Position;

    ClipSource();
    virtual ~ClipSource();

    inline void
    seekIfContainsFrame (const int64& f)
    {
        if (frames.contains (f))
            seekTrackFrame (f);
    }

    inline const FrameSpan& range() const { return frames; }
    inline bool isInRangeOf (const int64& frame) const
    {
        return frames.contains (frame);
    }

    inline int64 duration()    const { return frames.getLength(); }
    inline int64 frameStart()  const { return frames.getStart(); }
    inline int64 frameEnd()    const { return frames.getEnd(); }
    inline int64 frameLength() const { return frames.getLength(); }
    inline int32 frameOffset() const { return clipOffset; }

    ClipModel getModel() const;

    inline bool hasData() const { return data.get() != nullptr; }
    inline bool isEmpty() const { return ! hasData(); }

    /** Request the clip seek it's data in alignment with the track frame */
    inline void seekTrackFrame (const int64& trackFrame) { seekLocalFrame (trackFrame - frames.getStart()); }

    /** Request the clip seek it's data to it's local frame */
    virtual void seekLocalFrame (const int64& frame) = 0;

    inline void setNextReadPosition (int64 newPosition) { framePos.set (newPosition); }
    inline int64 getNextReadPosition() const { return framePos.get(); }
    inline int64 getTotalLength() const { return frames.getLength(); }
    inline bool isLooping() const { return looping; }
    inline void setLooping (bool shouldLoop) { looping = shouldLoop; }

    void valueChanged (Value &value);

    inline const ClipData* getClipData() const { return data.get(); }
    
protected:
    inline double getParentRate() const
    {
        return (double) parentRate.getValue();
    }

    inline virtual void setTime (const Range<double>& time) {
        setTimeSeconds (time);
    }

    inline void setTimeSeconds (const Range<double>& time) {
        setTimeSeconds (time.getStart(), time.getLength());
    }

    inline void setTimeSeconds (const double in, const double len) {
        setTimeFrames ((int64) std::floor (in * getParentRate()), llrint (len * getParentRate()));
    }

    inline void setTimeFrames (int64 in, int64 len)
    {
        frames.setStart  (in);
        frames.setLength (len);
    }

private:
    friend class ClipFactory;
    friend class Sequencer;
    friend class SequencerTrack;

    Value parentRate;
    ValueTree state;

    // realtime params
    Shared<ClipData> sdata;
    AtomicValue<ClipData*> data;

    inline bool
    setData (Shared<ClipData> d)
    {
        Shared<ClipData> old = sdata;
        sdata = d;

        bool res = data.set (sdata.get());
        if (! res)
            sdata = old;

        return res;
    }

    int32 clipOffset;
    FrameSpan frames;
    AtomicValue<int64> framePos;
    bool looping;

    // model listening, non realtime params
    Value start, length, offset;

    /** (un)register clip values for Value listening */
    void connectValues (bool disconnect = false);

    /** Set the clip model, ClipFactory will use this when creating new sources
        and restoring recycled clips */
    void setModel (const ClipModel& model);
};

class ClipBin :  public LinkedList<ClipSource>
{
public:

    ClipBin() { }
    ~ClipBin() { }
};

}

#endif // ELEMENT_CLIP_SOURCE_H
