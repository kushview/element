// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#ifndef ELEMENT_SEQUENCER_COMPONENT_H
#define ELEMENT_SEQUENCER_COMPONENT_H

#include <element/session.hpp>
#include "gui/Timeline.h"

namespace element {

class GuiApp;
class SequencerClipItem;

class SequencerComponent : public TimelineComponent,
                           public DragAndDropTarget,
                           public ValueTree::Listener
{
public:
    SequencerComponent (GuiApp& gui);
    ~SequencerComponent();

    int getNumTracks() const;

    void paintTrackHeader (Graphics& g, int track, const Rectangle<int>& area);
    void paintTrackLane (Graphics& g, int track, const Rectangle<int>& area);

    bool isInterestedInDragSource (const SourceDetails& details);
    void itemDragEnter (const SourceDetails& details);
    void itemDragMove (const SourceDetails& details);
    void itemDragExit (const SourceDetails& details);
    void itemDropped (const SourceDetails& details);
    bool shouldDrawDragImageWhenOver() { return false; }

protected:
    void timerCallback();

    void clipClicked (TimelineClip* clip, const MouseEvent& clipEvent);
    void clipDoubleClicked (TimelineClip* clip, const MouseEvent& clipEvent);
    void timelineBodyClicked (const MouseEvent& ev, int track);
    void timelineTrackHeadersClicked (const MouseEvent& ev, int track);

private:
    GuiApp& gui;
    SessionRef session;
    ValueTree state;

    Array<SequencerClipItem*> clipList;

    Shared<Monitor> pos;
    int dropTrack;

    friend class ValueTree;
    void valueTreePropertyChanged (ValueTree& tree, const Identifier& property);
    void valueTreeChildAdded (ValueTree& parent, ValueTree& child);
    void valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int);
    void valueTreeChildOrderChanged (ValueTree& parent, int, int);
    void valueTreeParentChanged (ValueTree& tree);
    void valueTreeRedirected (ValueTree& tree);

    void recycleSequencerClip (SequencerClipItem* clip);
};

} // namespace element

#endif /* ELEMENT_SEQUENCER_COMPONENT_H */
