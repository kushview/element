// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#if 0

#include "SequencerClipItem.h"
#include "SequencerComponent.h"

namespace element {

SequencerClipItem::SequencerClipItem (SequencerComponent& owner,
                                      const ClipModel& m)
    : TimelineClip (owner),
      sequence (owner),
      model (m)
{ }

void SequencerClipItem::paint (Graphics &g)
{
    g.setColour (Colours::whitesmoke);
    g.fillAll();
    g.setColour (Colours::darkgrey.darker());
}

void SequencerClipItem::getTime (Range<double>& time) const
{
    time.setStart (model.start());
    time.setEnd (model.end());
}

void
SequencerClipItem::reset()
{
    model.node() = ValueTree();
}

void SequencerClipItem::setTime (const Range<double>& time)
{
    model.setTime (time);
}

int SequencerClipItem::trackRequested (const int &track)
{
    return clampNoMoreThan (track, 0, sequence.getNumTracks() - 1);
}

void
SequencerClipItem::setModel (const ClipModel& clip)
{
    reset();
    model.node() = clip.node();
    repaint();
}

}
#endif
