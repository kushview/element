/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

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

#if 0

#include "gui/SequencerClipItem.h"
#include "gui/SequencerComponent.h"
#include "gui/GuiCommon.h"
#include "gui/ContentComponent.h"
#include "session/session.hpp"

namespace Element {

SequencerComponent::SequencerComponent (GuiApp &g)
    : gui (g)
{
    session = gui.session();
    setTrackWidth (120);

    pos = session->playbackMonitor();
    indicator()->setDragable (false);
    indicator()->setPosition (pos->get(), false);

    state.addListener (this);
    state = session->node();
    Value tempo = session->getPropertyAsValue (Slugs::tempo);
    getTempoValue().referTo (tempo);

    startTimer (60);
}

SequencerComponent::~SequencerComponent()
{
    state.removeListener (this);
    session.reset();
}

void SequencerComponent::clipClicked (TimelineClip *clip, const MouseEvent&)
{
    //ContentComponent* cc = findParentComponentOfClass<ContentComponent>();
    // cc->stabilize();
}

void SequencerComponent::clipDoubleClicked (TimelineClip *clip, const MouseEvent &clipEvent)
{
#if 0
    if (SequencerClipItem* c = dynamic_cast<SequencerClipItem*> (clip))
    {
        if (ContentComponent* cc = findParentComponentOfClass<ContentComponent>())
        {
            const String assetId = c->model.getProperty (Slugs::assetId);
            AssetItem item (session->assets().root().findItemForId (assetId));

            if (item.isValid())
            {
                Session::Track track (session->getTrack (clip->trackIndex()));
                // cc->showScreenForAsset (item);
            }
        }
    }
#endif
}

int SequencerComponent::getNumTracks() const
{
    return session->numTracks();
}

bool SequencerComponent::isInterestedInDragSource (const SourceDetails& details)
{
    File file (details.description.toString());
    return file.existsAsFile();
}

void SequencerComponent::itemDragEnter (const SourceDetails& details)
{
    dropTrack = trackAt (details.localPosition);
    if (dropTrack > session->numTracks())
        dropTrack = session->numTracks();
}

void SequencerComponent::itemDragMove (const SourceDetails& details)
{
    dropTrack = trackAt (details.localPosition);
    if (dropTrack > session->numTracks())
        dropTrack = session->numTracks();
}

void SequencerComponent::itemDragExit (const SourceDetails& details)
{
    dropTrack = -1;
}

void SequencerComponent::itemDropped (const SourceDetails& details)
{
    const bool droppedOnHeader = details.localPosition.getX() < getTrackWidth();

    if (droppedOnHeader)
        return;

    dropTrack = trackAt (details.localPosition);
    if (dropTrack > session->numTracks())
        dropTrack = session->numTracks();

    if (dropTrack == session->numTracks()) {
        session->appendTrack();
    }

    const File file (details.description.toString());
    Session::Track track = session->getTrack (dropTrack);
    track.addClip (file, xToTime (details.localPosition.getX ()));
}


void SequencerComponent::paintTrackHeader (Graphics &g, int track, const Rectangle<int> &area)
{
    Session::Track t (session->getTrack (track));

    if (t.isValid())
    {
        g.setColour (Colours::lightgrey);
        g.fillRect (area);

        g.setColour (Colours::darkcyan.contrasting (.95f));
        g.drawFittedText (t.getName(), area, Justification::left, 1);
    }
}

void SequencerComponent::paintTrackLane (Graphics&, int /*track*/, const Rectangle<int>& /*area*/)
{

}

void SequencerComponent::timerCallback()
{
    if (! pos)
        pos = session->playbackMonitor();

    if (pos)
        indicator()->setPosition (pos->get(), false);
}

void SequencerComponent::timelineBodyClicked (const MouseEvent &ev, int track)
{

}

void SequencerComponent::timelineTrackHeadersClicked (const MouseEvent &ev, int t)
{
    if (t < session->numTracks())
    {
        Session::Track track (session->getTrack (t));
        if (ev.mods.isPopupMenu())
        {
            PopupMenu menu;
            menu.addItem (1, "Remove Track");
            if (menu.show() == 1) {
                track.removeFromSession();
                repaint();
            }
        }
    }
    else
    {
        if (ev.mods.isPopupMenu())
        {
            PopupMenu menu;
            menu.addItem (1, "Add Audio Track");
            //menu.addItem (2, "Add MIDI Track");
            menu.addItem (3, "Add Pattern Track");

            const int res = menu.show();
            if (res >= 1 && res <= 3)
            {
                session->appendTrack (res == 1 ? "audio" : res == 2 ? "midi" : "pattern");
                repaint();
            }
        }
    }
}


void SequencerComponent::valueTreePropertyChanged (ValueTree& tree, const Identifier& property)
{

}

void SequencerComponent::valueTreeChildAdded (ValueTree& parent, ValueTree& child)
{
    if (parent.hasType (Slugs::sequence) && child.hasType (Slugs::track))
    {
        TrackModel t (child);
        triggerAsyncUpdate();
    }
    else if (parent.hasType (Slugs::track) && child.hasType (Slugs::clip))
    {
        SequencerClipItem* item = nullptr;

        TrackModel track (parent);
        ClipModel model (child);
        assert (model.node().hasProperty ("media"));

        const Identifier type (model.node().getProperty ("media", String()));

        if (type == Slugs::pattern)
        {

        }
        else {
            item = findFreeClip<SequencerClipItem>();
            if (! item)
                item = new SequencerClipItem (*this, child);
        }

        if (nullptr != item)
        {
            item->setModel (model);
            addTimelineClip (item, track.index());
            clipList.add (item);
        }
    }
}

void SequencerComponent::valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int)
{
    if (parent.hasType (Slugs::sequence) && child.hasType (Slugs::track))
    {
        valueTreeRedirected (parent);
        triggerAsyncUpdate();
    }
    else if (parent.hasType (Slugs::track) && child.hasType (Slugs::clip))
    {
        ClipModel clip (child);
        for (SequencerClipItem* c : clipList)
            if (c->model == clip)
                { recycleSequencerClip (c); break; }
    }
}

void SequencerComponent::valueTreeChildOrderChanged (ValueTree& parent, int, int)
{
    if (parent == this->state) {
        valueTreeRedirected (parent);
    }
}

void SequencerComponent::valueTreeParentChanged (ValueTree& tree)
{

}

void SequencerComponent::valueTreeRedirected (ValueTree &tree)
{
    if (tree != this->state)
        return;

    for (SequencerClipItem* c : clipList)
        recycleSequencerClip (c);

    ScopedPointer<Session::Track> track = new Session::Track (session->getTrack (0));
    while (track && track->isValid())
    {
        ValueTree state = track->state();
        for (int c = 0; c < state.getNumChildren(); ++c)
        {
            ValueTree child (state.getChild (c));
            valueTreeChildAdded (state, child);
        }
        track = new Session::Track (track->next());
    }
}

void SequencerComponent::recycleSequencerClip (SequencerClipItem* clip)
{
    clipList.removeAllInstancesOf (clip);
    clip->reset();
    recycleClip (clip);
}

}

#endif
