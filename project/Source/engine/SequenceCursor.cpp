/*
    SequenceCursor.cpp
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

#if JUCE_COMPLETION
#include "modules/element_engines/element_engines.h"
#endif

// Constructor.
SequenceCursor::SequenceCursor (Sequencer& seq, int64 frame, DataType sync_type)
    : owner (seq),
      mSyncType (sync_type),
      trackCount (0)
{
    mFrame = frame;
    rtClipSize = 0;

    resetClips();
	reset();
}


// Destructor.
SequenceCursor::~SequenceCursor()
{
    //destroyed();
    //owner.removeCursor (this);
    rtClips.reset();
}


// Clip sync flag accessor.
void
SequenceCursor::setSyncType (DataType sync)
{
    mSyncType = sync;
}

DataType
SequenceCursor::syncType() const
{
    return mSyncType;
}


// General bi-directional locate method.
void
SequenceCursor::seek (int64 frame, bool sync)
{
#if 1
    if (frame == mFrame)
        return;

    const int32 ntracks = owner.tracks().size();
    int32 index = 0;

    while (index < ntracks && index < trackCount.get())
    {
        const SequencerTrack* track (owner.tracks().getUnchecked (index));
        ClipSource* clip = nullptr;
        ClipSource* lastClip = rtClips [index];

        // Optimize if seeking forward...
        if (frame > mFrame)
            clip = lastClip;

        // Locate first clip not past the target frame position..
        clip = seekClip (track, clip, frame);

        // Update cursor track clip...
        rtClips [index] = clip;

#if 1
        // Now something fulcral for clips around...
        // FIXME: if (track->sync_type() == m_sync_type)
        {
            // Tell whether play-head is after loop-start position...
            // FIXME: bool is_looping = (frame >= p_seq->loop_start());
            // const bool is_looping = frame > 0;

            // Care for old/previous clip...
            if (lastClip && lastClip != clip)
            {
               // lastClip->reset (is_looping);
            }

            // Set final position within target clip...
            if (clip && sync)
            {
#if 0
                // Take care of overlapping clips...
                const uint64_t clip_end = clip->frame_end();

                Clip::iterator c = clip->position();
                Clip::iterator e = track->clips_end();
                while (c != e)
                {
                    Clip& clip (*c);

                    uint64_t clip_start = clip.frame_start();

                    if (clip_start > clip_end)
                    {
                        break;
                    }

                    if (frame >= clip_start && frame < clip_start + clip.duration())
                    {
                        clip.seek (frame - clip_start);
                    }
                    else
                    {
                        clip.reset (is_looping);
                    }

                    ++c;
                }
#endif
            }
        }
#endif
        ++index;
	}
#endif

    mFrame = frame;
}

int64
SequenceCursor::frame() const
{
    return mFrame;
}

void
SequenceCursor::setFrameTime (int64 frameTime)
{
    mFrameTime = frameTime;
    mFrameDelta = mFrame - frameTime;
}

int64
SequenceCursor::frameTime() const
{
    return mFrameTime;
}

int64
SequenceCursor::setFrameTimeExpected() const
{
    return mFrameTime + mFrameDelta;
}


ClipSource*
SequenceCursor::clip (int32 track) const
{
    return (track < trackCount.get() ? rtClips [track] : nullptr);
}

ClipSource*
SequenceCursor::seekClip (const SequencerTrack* track, ClipSource* clip, int64 frame) const
{
    if (clip == nullptr)
        clip = track->firstClip();

    while (clip && frame > clip->frameEnd())
        clip = clip->next();

    if (clip == nullptr)
        clip = track->lastClip();

    return clip;
}


void
SequenceCursor::addTrack()
{
    const int32 newCount = trackCount.get() + 1;

    if (rtClipSize < newCount)
    {
        rtClipSize += newCount;
        ClipArray newSources (new ClipSource* [rtClipSize]);
        updateClips (newSources.get(), newCount);

        rtClips.swap (newSources);
    }
    else
    {
        updateClips (rtClips.get(), newCount);
    }

    while (! trackCount.set (newCount)) { /*spin*/ }

}

void
SequenceCursor::updateTrack (const SequencerTrack* track)
{
    const int32 index = track->index();
    if (isPositiveAndBelow (index, trackCount.get()))
    {
        ClipSource* clip = seekClip (track, nullptr, mFrame);
        if (clip && clip->isInRangeOf (mFrame))
            clip->seekLocalFrame (mFrame - clip->frameStart());

        rtClips [index] = clip;
    }
}

void
SequenceCursor::removeTrack (const SequencerTrack* track)
{
    const int index = track->index();
    if (index >= 0 && uint32_t (index) < numTracks())
        removeTrack ((uint32_t) index);
}

void
SequenceCursor::removeTrack (int32 track)
{
    if (trackCount.set (trackCount.get() - 1))
    {
        for ( ; track < numTracks(); ++track)
            rtClips [track] = rtClips [track + 1];
        rtClips [track] = nullptr;
    }
}

void
SequenceCursor::updateTrackClip (const SequencerTrack* track)
{
    const int index = track->index();
    if (index >= 0)
    {
        ClipSource* clip = rtClips [index];
#if 1
        if (clip)
        {
            if (clip->isInRangeOf (mFrame))
                clip->seekLocalFrame (mFrame - clip->frameStart());
            else
            {
                // XXX: clip->reset ();
			}
		}
#endif
	}
}

void
SequenceCursor::updateClips (ClipSource** clips, int32 size)
{
    const Array<SequencerTrack*>& tracks (owner.tracks());
    const int32 tsize = tracks.size();

    int32 index = 0;
    while (index < size && index < tsize)
    {
        ClipSource* clip = seekClip (tracks.getUnchecked (index),
                                     nullptr, mFrame);

        if (clip && clip->isInRangeOf (mFrame))
            clip->seekLocalFrame (mFrame - clip->frameStart());

        clips [index] = clip;
        ++index;
    }

}

void
SequenceCursor::reset()
{
    mFrameTime = 0;
    mFrameDelta = mFrame;
}

void
SequenceCursor::resetClips()
{
    const uint32_t ntracks = owner.numTracks();
    rtClipSize = (ntracks << 1);

    if (rtClipSize > 0)
    {
        ClipArray newClips (new ClipSource* [rtClipSize]);
        updateClips (newClips.get(), ntracks);
        rtClips.swap (newClips);
    }

    trackCount.set (ntracks);
}

void
SequenceCursor::process (int32 nframes)
{
    mFrameTime += nframes;
}
