/*
    SequencerClipItem.h - This file is part of Element
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

#ifndef ELEMENT_SEQUENCER_CLIP_ITEM_H
#define ELEMENT_SEQUENCER_CLIP_ITEM_H

#include "gui/Timeline.h"

namespace Element {
namespace Gui {

class SequencerComponent;

class SequencerClipItem :  public TimelineClip
{
public:

    SequencerClipItem (SequencerComponent& owner, const ClipModel& clipData);
    virtual ~SequencerClipItem() { }
    
    void paint (Graphics &g);

    virtual void getTime (Range<double>& time) const;
    virtual void setTime (const Range<double>& time);

    int trackRequested (const int &track);

    void setModel (const ClipModel& clip);
    void reset();

    void
    mouseDoubleClick (const MouseEvent &event)
    {
        TimelineClip::mouseDoubleClick (event);
    }

    inline int32 trackIndex() const { return model.trackIndex(); }

    virtual int32 trackRequested (const int32 t)
    {
        TrackModel track (model.node().getParent());
        const int32 currentIndex = trackIndex();
        if (track.isValid())
        {
            TrackModel next (track.state().getSibling (t - currentIndex));
            if (next.isValid())
            {
                track.state().removeChild (model.node(), nullptr);
                next.state().addChild (model.node(), -1, nullptr);
            }
        }

        return trackIndex();
    }

protected:

    SequencerComponent& sequence;
    ClipModel model;

private:

    friend class SequencerComponent;
    Range<double> timespan;

};

}}

#endif /* ELEMENT_SEQUENCER_CLIP_ITEM_H */
