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

#ifndef ELEMENT_MIDI_EDITOR_BODY_H
#define ELEMENT_MIDI_EDITOR_BODY_H

#include "gui/NoteClipItem.h"

namespace Element {

class NoteSequence;
    
class NoteSelection :  public SelectedItemSet<NoteClipItem*>
{
public:

    NoteSelection();
    virtual ~NoteSelection();
    void itemSelected (NoteClipItem *item);
    void itemDeselected (NoteClipItem *item);

};

class MidiEditorBody :  public TimelineComponent,
                        public LassoSource<NoteClipItem*>,
                        private ValueTree::Listener
{
public:

    MidiEditorBody (MidiKeyboardState& keyboard);
    ~MidiEditorBody();

    void showAllTracks();
    void hideEmptyKeys();

    inline bool triggerNotes() const { return (var)shouldTriggerNotes; }
    inline Value triggerNotesValue() { return shouldTriggerNotes; }

    //======================================================================
    /** Set the Note sequence to use */
    void setNoteSequence (const NoteSequence& s);

    /** Show only notes for a given MIDI channel

        @param channel The MIDI channel (1-16)
        @param updateInsertChannel Update the insert channel

        @notes Setting channel to 0 will show all notes regardless of
        channel
    */
    void setVisibleChannel (int channel, bool updateInsertChannel = true);
    bool channelIsVisible (int channel) const { return midiChannels [channel]; }

    //======================================================================
    virtual void findLassoItemsInArea (Array <NoteClipItem*>& itemsFound, const Rectangle<int>& area);
    NoteSelection& getLassoSelection();

    //======================================================================
    void mouseDoubleClick (const MouseEvent &event);
    void mouseDrag (const MouseEvent &event);
    void mouseUp (const MouseEvent& ev);
    void mouseWheelMove (const MouseEvent &event, const MouseWheelDetails &wheel);

protected:

    template<class Handler>
    inline void foreachNote (Handler handle)
    {
        for (int i = 0; i < notes.size(); ++i)
            if (NoteClipItem* note = notes.getUnchecked(i))
                handle (note);
    }

    /** Add a note to the editor. */
    void addNote (int note, float beat, float length = 1.0f, int channel = 1);

    /** Add a midi sequence to the editor */
    void addSequence (const MidiMessageSequence& seq);

    /** @internal */
    void onNoteAdded (const Note& note);

    /** @internal */
    void onNoteRemoved (const Note& note);

    /** Select all of the notes for a given keyboard key
        @param key The note on the keyboard
        @param deselectOthers If true all other notes deselect
    */
    void selectNotesOnKey (int key, bool deselectOthers);

    /** Unload a note.  Frees the model and recycles the clip */
    void unloadNote (NoteClipItem* clip);

    //======================================================================
    void clipClicked (TimelineClip* clip, const MouseEvent& clipEvent);
    void clipDoubleClicked (TimelineClip* clip, const MouseEvent& /* clipEvent */);
    void clipMoved (TimelineClip* clip, const MouseEvent&, double deltaStart, double deltaEnd);
    void clipChangedTrack (TimelineClip *clip, int trackDelta);
    int getNumTracks() const;
    void timelineBodyClicked (const MouseEvent& ev, int track);
    void timelineTrackHeadersClicked (const MouseEvent &ev, int track);
    virtual void paintTrackHeader (Graphics &g, int track, const Rectangle<int> &area);
    virtual void paintTrackLane (Graphics &g, int track, const Rectangle<int> &area);
    virtual void refreshComponentForTrack (const int track);

private:

    ValueTree sequenceNode;

    MidiKeyboardState& keyboardState;

    LassoComponent<NoteClipItem*> lasso;
    NoteSelection selected;

    bool trackDrag;
    bool keyboardDrag;
    int trackDeltaY, dragTrack;

    Array<NoteClipItem*> notes;

    //Signal<void()> changedSignal;
    OptionalScopedPointer<NoteSequence> sequence;

    ValueTree props;
    Value shouldTriggerNotes;

    int insertChannel;
    float insertLength, insertVelocity;
    BigInteger midiChannels;

    friend class ValueTree;
    void valueTreeChildAdded (ValueTree& parent, ValueTree& child);
    void valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int);
    virtual void valueTreePropertyChanged (ValueTree& tree, const Identifier& property);
    virtual void valueTreeChildOrderChanged (ValueTree& parent, int, int);
    virtual void valueTreeParentChanged (ValueTree& child);
};

}

#endif // ELEMENT_MIDI_EDITOR_BODY_H
