/*
    MidiEditorBody.cpp - This file is part of Element
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

#include <boost/bind.hpp>
#include "session/NoteSequence.h"
#include "gui/MidiEditorBody.h"

namespace Element {

typedef SelectedItemSet<NoteClipItem*>::ItemArray SelectedNotes;

NoteSelection::NoteSelection() { }
NoteSelection::~NoteSelection() { }

void
NoteSelection::itemSelected (NoteClipItem *item)
{
    item->setSelected (true);
    item->repaint();
}

void
NoteSelection::itemDeselected  (NoteClipItem *item)
{
    item->setSelected (false);
    item->repaint();
}

//=========================================================================
MidiEditorBody::MidiEditorBody (MidiKeyboardState& k)
    : keyboardState (k)
{
    shouldTriggerNotes.setValue (false);
    sequence.setOwned (new NoteSequence());
    sequenceNode = sequence->node();
    sequenceNode.addListener (this);

    setTrackWidth (80);
    setTrackHeightsOffset (-getTracksTotalHeight() / 2, false);
    resized();
    repaint();

    addAndMakeVisible (&lasso);
    trackDeltaY = 0;

    insertChannel = 1;
    insertLength = 0.49f;
    insertVelocity = 0.8f;

    trackDrag = keyboardDrag = false;
}

MidiEditorBody::~MidiEditorBody()
{
    sequenceNode.removeListener (this);
    notes.clear();
    sequence.clear();
}


void
MidiEditorBody::addNote (int note, float start, float length, int channel)
{
    assert (sequence.get() != nullptr);
    sequence->addNote (note, start, length, channel);
}


void MidiEditorBody::addSequence (const MidiMessageSequence&)
{
}

void MidiEditorBody::clipClicked (TimelineClip* clip, const MouseEvent& clipEvent )
{
    if (NoteClipItem* n = dynamic_cast<NoteClipItem*> (clip))
    {
        if (selected.getNumSelected() == 0)
            selected.selectOnly (n);
        else if (clipEvent.mods.isShiftDown())
            selected.addToSelection (n);

        if (triggerNotes())
            keyboardState.noteOn (insertChannel, n->keyId(), insertVelocity);
    }
}

void
MidiEditorBody::clipDoubleClicked (TimelineClip* clip, const MouseEvent& /* clipEvent */)
{
    if (NoteClipItem* n = dynamic_cast<NoteClipItem*> (clip))
        sequence->removeNote (n->note());
}

void MidiEditorBody::clipMoved (TimelineClip* clip, const MouseEvent&, double deltaStart, double deltaEnd)
{
    if (clip->isSelected())
    {
        Range<double> time;
        
        const SelectedNotes& items (selected.getItemArray());
        for (int i = 0; i < items.size(); ++i)
        {
            NoteClipItem* note = items.getUnchecked(i);
            if (note == clip)
                continue;

            note->getTime (time);
            time.setStart (time.getStart() + deltaStart);
            time.setEnd (time.getEnd() + deltaEnd);
            note->setTime (time);

            updateClip (note);
        }
    }
}

void
MidiEditorBody::clipChangedTrack (TimelineClip *clip, int trackDelta)
{

    if (triggerNotes()) {
        if (NoteClipItem* note = dynamic_cast<NoteClipItem*> (clip))
            keyboardState.noteOn (note->channel(), note->keyId(), insertVelocity);
    }

    if (! clip->isSelected())
        return;

    const SelectedNotes& notes (selected.getItemArray());
    for (int i = 0; i < notes.size(); ++i)
    {
        NoteClipItem* note = notes.getUnchecked (i);
        if (note == clip) {
             continue;
        }

        note->adjustTrackIndex (trackDelta, false);
        updateClip (note);
    }
}


void
MidiEditorBody::findLassoItemsInArea (Array <NoteClipItem*>& itemsFound, const Rectangle<int>& area)
{
    for (int i = 0; i < notes.size(); ++i)
    {
        NoteClipItem* note = notes.getUnchecked(i);
        if (area.intersects (note->getBounds())) {
            itemsFound.add (note);
            selected.addToSelection (note);
        } else {
            selected.deselect (note);
        }
    }
}

NoteSelection& MidiEditorBody::getLassoSelection() { return selected; }
int MidiEditorBody::getNumTracks() const { return 127; }

void
MidiEditorBody::mouseDoubleClick (const MouseEvent &ev)
{
    if (ev.x > getTrackWidth())
    {
        const int keyId = 127 - trackAt (ev);
        const double ticks (xToTicks (ev.x, true));

        addNote (keyId, ticks, (float)1920.f * 0.5f, insertChannel);
    }
}

void
MidiEditorBody::mouseDrag (const MouseEvent &event)
{
    lasso.dragLasso (event);

    if (trackDrag)
    {
        setTrackHeightsOffset ( -(trackDeltaY - event.y), true);
        trackDeltaY = event.y;
    }

    if (keyboardDrag)
    {
        const int keyId = 127 - trackAt (event);
        selectNotesOnKey (keyId, false);

        if (dragTrack != trackAt (event))
        {
            dragTrack = trackAt (event);

            if (triggerNotes())
                keyboardState.noteOn (insertChannel, 127 - dragTrack, insertVelocity);
        }
    }
}

void MidiEditorBody::mouseUp (const MouseEvent& ev)
{
    TimelineComponent::mouseUp (ev);
    lasso.endLasso();
    trackDrag = keyboardDrag = false;
    trackDeltaY = 0;
    keyboardState.allNotesOff (insertChannel);
}

void MidiEditorBody::mouseWheelMove (const MouseEvent &event, const MouseWheelDetails &wheel)
{
    if (this == getComponentAt (event.getPosition()))
        setTrackHeightsOffset (wheel.deltaY * 100.0f, true);
}

void MidiEditorBody::onNoteAdded (const Note& note)
{
    if (note.isValid())
    {
        NoteClipItem* c = findFreeClip<NoteClipItem>();

        if (c == nullptr)
            c = new NoteClipItem (*this, note);
        else {
            c->setModel (note);
        }

        notes.add (c);
        addTimelineClip (c, 127 - c->keyId());
    }
}

void MidiEditorBody::onNoteRemoved (const Note& note)
{
    for (int i = 0; i < notes.size(); ++i)
    {
        NoteClipItem* const clip = notes.getUnchecked (i);
        if (clip->note() == note)
        {
            notes.removeFirstMatchingValue (clip);
            unloadNote (clip);
            break;
        }
    }
}

void MidiEditorBody::showAllTracks()
{
    BigInteger keys;
    keys.setRange (0, 128, true);
    setTrackVisibility (keys);
}

void MidiEditorBody::hideEmptyKeys()
{
    BigInteger keys;
    keys.setRange (0, 128, false);
    sequence->getKeysWithEvents (keys);
    setTrackVisibility (keys);
}

void MidiEditorBody::setNoteSequence (const NoteSequence& s)
{
    if (sequenceNode == s.node())
            return;

    sequence.setOwned (new NoteSequence (s));
    sequenceNode = sequence->node();

    selected.deselectAll();
    foreachNote (boost::bind (&MidiEditorBody::unloadNote, this, ::_1));
    notes.clear();

    for (int32 c = sequenceNode.getNumChildren(); --c >= 0;)
    {
        Note note (sequenceNode.getChild (c));
        onNoteAdded (note);
    }

    repaint();
}

void MidiEditorBody::paintTrackHeader (Graphics &g, int track, const Rectangle<int> &area)
{
    g.setColour (Colours::darkgrey);
    g.fillRect (area);

    track = (127 - track);

    if (track < 0)
        return;

    if (track % 12 == 0)
    {
        g.setFont (Font (10.0f));
        g.setColour (Colours::black.withAlpha (0.78f));
        g.drawText (Midi::noteToText (track), area, Justification::left, true);

        g.setColour (Colours::black.withAlpha (0.5f));
        g.drawHorizontalLine (area.getBottom(), area.getX(), area.getRight());
    }

    if (Midi::noteIsWhiteKey (track))
        g.setColour (Colours::white);
    else
        g.setColour (Colours::black);

    Rectangle<int> r (area);
    g.fillRect (r.removeFromRight (area.getWidth() / 2));
}

void MidiEditorBody::paintTrackLane (Graphics &g, int track, const Rectangle<int> &area)
{
#if 1
    track = (127 - track);

    g.resetToDefaultState();

    if (Midi::noteIsWhiteKey (track))
        g.setColour (Colour (0xff999999));
    else
        g.setColour (Colour (0xff888888));

    g.fillRect (area);
#endif
}


void MidiEditorBody::selectNotesOnKey (int key, bool deselectOthers)
{
    for (int i = 0; i < notes.size(); ++i)
    {
        NoteClipItem* const n = notes.getUnchecked (i);
        if (n->keyId() == key && n->channel() == insertChannel)
            selected.addToSelection (n);
        else if (deselectOthers)
            selected.deselect (n);
    }
}

void MidiEditorBody::setVisibleChannel (int chan, bool updateInsertChannel)
{
    if (chan == 0)
    {
        midiChannels.setRange (1, 16, true);
        for (int i = 0; i < notes.size(); ++i)
        {
            NoteClipItem* const c = notes.getUnchecked (i);
            c->setVisible (true);
            updateClip (c);
        }
        return;
    }

    midiChannels.setBit (clampNoMoreThan (chan, 1, 16), true);
    insertChannel = updateInsertChannel ? chan : insertChannel;

    for (int i = 0; i < notes.size(); ++i)
    {
        NoteClipItem* const c = notes.getUnchecked (i);
        if (c->channel() == chan) {
            c->setVisible (true);
            updateClip (c);
        }
        else {
            c->setVisible (false);
        }
    }

    repaint();
}

void MidiEditorBody::timelineBodyClicked (const MouseEvent& ev, int track)
{
    PopupMenu menu;

    const int keyId = 127 - track;

    if (! ev.mods.isAnyModifierKeyDown())
    {
        selected.deselectAll();
    }

    if (ev.mods.isPopupMenu()) {
        menu.addSectionHeader (String ("MIDI Note ") + String(keyId));
        menu.show();
        return;
    }

    if (ev.mods.isCommandDown()) {
        addNote (keyId, xToTicks (ev.x, true), insertLength, insertChannel);
    }

    lasso.beginLasso (ev, this);
}

void MidiEditorBody::timelineTrackHeadersClicked (const MouseEvent &ev, int track)
{
    const int keyId = 127 - track;

    if (ev.x < getTrackWidth() / 2) {
        trackDrag = true;
        trackDeltaY = ev.y;
    } else {
        dragTrack = 0;
        keyboardDrag = true;
        selectNotesOnKey (keyId, ! ev.mods.isShiftDown());
        if (triggerNotes())
            keyboardState.noteOn (insertChannel, keyId, (float) insertVelocity);
    }
}

void MidiEditorBody::refreshComponentForTrack (const int track) { }

void MidiEditorBody::unloadNote (NoteClipItem* clip)
{
    selected.deselect (clip);
    clip->reset();
    recycleClip (clip);
}


void MidiEditorBody::valueTreeChildAdded (ValueTree& parent, ValueTree& child)
{
    if (parent == sequence->node() && child.hasType (Slugs::note)) {
        Note note (child);
        onNoteAdded (note);
    }
}

void MidiEditorBody::valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int)
{
    if (parent == sequence->node() && child.hasType (Slugs::note)) {
        Note note (child);
        onNoteRemoved (note);
    }
}

void MidiEditorBody::valueTreePropertyChanged (ValueTree& tree, const Identifier& property) {}
void MidiEditorBody::valueTreeChildOrderChanged (ValueTree& parent, int, int) {}
void MidiEditorBody::valueTreeParentChanged (ValueTree& child) {}

}

