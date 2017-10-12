/*
    Note.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
*/

#ifndef ELEMENT_NOTE_H
#define ELEMENT_NOTE_H

#include "ElementApp.h"

namespace Element {

    class Note;
    class Note :  public ObjectModel
    {
    public:

        struct EditDeltas
        {
            int note, channel;
            float velocity;
            double start, length;

            EditDeltas()
                : note (0), channel (0),
                  velocity (0.0), start (0.0),
                  length (0.0) { }

            ~EditDeltas() { }

            inline void reset()
            {
                EditDeltas zero;
                this->operator= (zero);
            }

            inline bool isZero() const
            {
                return false;
            }

            inline EditDeltas& operator= (const EditDeltas& other)
            {
                this->length = other.length;
                this->note = other.note;
                this->start = other.start;
                this->velocity = other.velocity;
                return *this;
            }
        };

        Note (const Note& n) : ObjectModel (n.node()) { }
        Note (const ValueTree& data) : ObjectModel (data) { }

        static inline Note
        make (const MidiMessage& midi, double beatLength)
        {
            if (! midi.isNoteOn())
                return Note (ValueTree::invalid);

            float velocity = (float)midi.getVelocity() / 127.f;
            return Note (midi.getNoteNumber(), midi.getTimeStamp(),
                         beatLength, midi.getChannel(), velocity);
        }

        static inline Note
        make (int note, double beat, double length = 1.0f, int channel = 1, float velocity = 0.8f) {
            return Note (note, beat, length, channel, velocity);
        }

        static inline Note make (const ValueTree& tree) {
            return Note (tree);
        }

        static inline Note make (const ObjectModel& object) {
            return make (object.node());
        }


        ~Note() { }

        /** Interfaces that deal with notes can set this id to be used
            in fast note lookup ops. See PatternInterface for an example.
            Usually, an interface will set this Id to something relavent
            when the Inferface-Side note is newly created.

            This funtion is thread/realtime safe.

            @param id The new ID to use
        */
        void setEventId (const int id) const;

        bool isValid() const;

        /** The event index in the interface-side implementation. */
        const int eventId() const;

        /** Returns the current key id (midi note) for this model */
        const int keyId() const;

        /** Returns the channel of this note */
        const int channel() const;

        /** Returns the velocity of this note as a ratio (0.0f to 1.0f) */
        const float velocity() const;

        /** The start position in beats */
        const double tickStart() const;

        /** Length of note in beats */
        const double beatLength() const;

        /** End of the note */
        const double tickEnd() const;

        /** Get the timings of this note as a Range */
        void getBeats (Range<double>& beats) const;

        /** Resize this notes length in beats
            Note that this does not change the start time */
        void resize (EditDeltas& changes, double length);

        /** Change the length */
        void changeLength (EditDeltas& d, double len);

        /** Change channel */
        void changeChannel (EditDeltas& changes, int c);

        /** Change the key id for this note */
        void changeKeyId (EditDeltas& changes, int key);

        /** Change this notes velocity */
        void changeVelocity (EditDeltas& changes, float vel);

        /** Move this note to a new start beat
            Length is unaffected */
        void move (EditDeltas& changes, const double beat);

        /** Apply pending changes to this note */
        void applyEdits (EditDeltas& changes, bool reset = true);

        MidiMessage noteOn() const;
        MidiMessage noteOff() const;
        void getMidi (MidiMessage& on, MidiMessage& off) const;
        ValueTree sequenceNode() const;

        inline bool operator== (const Note& n) const { return node() == n.node(); }
        inline bool operator!= (const Note& n) const { return node() != n.node(); }

    protected:

        friend class NoteSequence;
        Note (int note, double start, double length = 1.0f, int channel = 1, float velocity = 0.8f);

    };

}

#endif // ELEMENT_NOTE_H
