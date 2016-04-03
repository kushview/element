/*
    SequencerTrack.cpp - This file is part of Element
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

SequencerTrack::SequencerTrack (Sequencer& o, const TrackModel& t)
    : sequencer (o)
{
    track = new TrackModel (t);
    state = track->state();

    armed.referTo (track->armedValue());
    muted.referTo (track->mutedValue());
    soloed.referTo (track->soloedValue());
    volume.referTo (track->volumeValue());

    bin.setScoped (true);

    setPlayConfigDetails (2, 2, o.getSampleRate(), o.getBlockSize());
}

SequencerTrack::~SequencerTrack()
{

}

ClipSource* SequencerTrack::cursorClip() const
{
    return sequencer.trackClip (trackIndex);
}

void SequencerTrack::prepareToPlay (double sampleRate, int blockSize)
{
    setPlayConfigDetails (getTotalNumInputChannels(), getTotalNumOutputChannels(),
                          sampleRate, blockSize);

    ClipSource* clip = bin.first();
    while (clip)
    {
        clip->prepareToPlay (blockSize, sampleRate);
        clip = clip->next();
    }
}

void SequencerTrack::releaseResources()
{
    ClipSource* clip = bin.first();
    while (clip)
    {
        clip->releaseResources();
        clip = clip->next();
    }
}

void SequencerTrack::processBlock (AudioSampleBuffer &buffer, MidiBuffer &midi)
{
    if (! sequencer.position().isPlaying)
        return;

    buffer.clear();
    if (ClipSource* src = cursorClip())
    {
        const int32 clipStart = sequencer.position().timeInSamples - src->frameStart();
        const int32 clipEnd   = clipStart + buffer.getNumSamples();

        if (clipEnd <= 0)
            return;

        AudioSourceChannelInfo info (buffer);

        if (clipStart < 0)
        {
            src->setNextReadPosition (0);
            info.numSamples = clipStart + buffer.getNumSamples();
            info.startSample = -clipStart;
            src->getNextAudioBlock (info);
        }
        else if (clipStart >= 0 && clipEnd <= src->frameEnd())
        {
            src->setNextReadPosition (clipStart);
            src->getNextAudioBlock (info);
        }
        else if (clipStart < src->frameEnd() && clipEnd >= src->frameEnd())
        {
            src->setNextReadPosition (0);
            info.numSamples = src->frameEnd() - clipStart;
            info.startSample = 0;
            src->getNextAudioBlock (info);
        }
    }
}

void SequencerTrack::processBlockBypassed (AudioSampleBuffer&, MidiBuffer&)
{

}
