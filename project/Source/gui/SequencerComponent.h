/*
    SequencerComponent.h - This file is part of Element
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

#ifndef ELEMENT_SEQUENCER_COMPONENT_H
#define ELEMENT_SEQUENCER_COMPONENT_H

#include "session/Session.h"
#include "gui/Timeline.h"

namespace Element {
namespace Gui {

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

        void paintTrackHeader (Graphics &g, int track, const Rectangle<int> &area);
        void paintTrackLane (Graphics &g, int track, const Rectangle<int> &area);

        bool isInterestedInDragSource (const SourceDetails& details);
        void itemDragEnter (const SourceDetails& details);
        void itemDragMove (const SourceDetails& details);
        void itemDragExit (const SourceDetails& details);
        void itemDropped (const SourceDetails& details);
        bool shouldDrawDragImageWhenOver() { return false; }

    protected:
        void timerCallback();

        void clipClicked (TimelineClip *clip, const MouseEvent &clipEvent);
        void clipDoubleClicked (TimelineClip *clip, const MouseEvent &clipEvent);
        void timelineBodyClicked (const MouseEvent &ev, int track);
        void timelineTrackHeadersClicked (const MouseEvent &ev, int track);

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
        void valueTreeRedirected (ValueTree &tree);

        void recycleSequencerClip (SequencerClipItem* clip);
    };

}}

#endif /* ELEMENT_SEQUENCER_COMPONENT_H */
