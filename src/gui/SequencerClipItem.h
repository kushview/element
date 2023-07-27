// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#ifndef ELEMENT_SEQUENCER_CLIP_ITEM_H
#define ELEMENT_SEQUENCER_CLIP_ITEM_H

#include "gui/Timeline.h"

namespace element {

class SequencerComponent;

class SequencerClipItem : public TimelineClip
{
public:
    SequencerClipItem (SequencerComponent& owner, const ClipModel& clipData);
    virtual ~SequencerClipItem() {}

    void paint (Graphics& g);

    virtual void getTime (Range<double>& time) const;
    virtual void setTime (const Range<double>& time);

    int trackRequested (const int& track);

    void setModel (const ClipModel& clip);
    void reset();

    void
        mouseDoubleClick (const MouseEvent& event)
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

} // namespace element

#endif /* ELEMENT_SEQUENCER_CLIP_ITEM_H */
