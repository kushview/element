/*
    SequenceCursor.h.hpp - This file is part of Element
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

#ifndef ELEMENT_SEQUENCE_CURSOR_H
#define ELEMENT_SEQUENCE_CURSOR_H



class ClipSource;
class Sequencer;
class SequencerTrack;

class SequenceCursor
{
public:

	// Constructor.
    SequenceCursor (Sequencer& seq, int64 frame = 0, DataType type = DataType::Audio);
    ~SequenceCursor();

    /** Bi-directional locate
        @param frame The audio frame to seek
        @param sync If true clips will reset and/or seek to frame
     */
    void seek (int64 frame, bool sync = false);

    /** Get the current clip associated with frame()
        Returns the current clip for a given track */
    ClipSource* clip (int32 track) const;

    /** Returns the current frame (playhead) index */
    int64 frame() const;

    void setFrameTime (int64 frame);
    int64 frameTime() const;
    int64 setFrameTimeExpected() const;

    void setSyncType (DataType sync);
    DataType syncType() const;

    void addTrack();
    //int32 indexOfTrack (const SequencerTrack* track) const { return trackRefs.indexOf (track); }
    void removeTrack (const SequencerTrack* track);
    void updateTrack (const SequencerTrack* track);
    void updateTrackClip (const SequencerTrack* track);
    int32 numTracks() const { return trackCount.get(); }

    void reset();
    void resetClips();
    void process (int32 nframes);

    const Array<SequencerTrack*>& tracks() const { return trackRefs; }

protected:

    typedef std::unique_ptr<ClipSource * []> ClipArray;

    ClipSource* seekClip (const SequencerTrack* track, ClipSource* clip, int64 frame) const;
    void updateClips (ClipSource** clips, int32 totalTracks);
    void removeTrack (int32 track);

private:

    Sequencer&  owner;

    int64       mFrame;
    int64       mFrameTime;
    int64       mFrameDelta;
    DataType    mSyncType;

    AtomicValue<int32> trackCount;
    Array<SequencerTrack*> trackRefs;

    ClipArray  rtClips;
    int32      rtClipSize;
};

#endif  /* ELEMENT_SEQUENCE_CURSOR_H */
