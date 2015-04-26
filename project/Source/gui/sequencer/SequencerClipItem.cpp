/*
    SequencerClipItem.cpp - This file is part of Element
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

#include "SequencerClipItem.h"
#include "SequencerComponent.h"

namespace Element {
namespace Gui {


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
    model.node() = ValueTree::invalid;
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
}
