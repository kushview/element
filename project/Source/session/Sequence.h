/*
    Session.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.

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

#ifndef EL_SEQUENCE_H
#define EL_SEQUENCE_H

#include "element/Juce.h"
#include "session/AssetTree.h"
#include "session/ClipModel.h"
#include "session/TrackModel.h"

namespace Element {
    class Session;
    class EngineControl;
    class Globals;
    class Instrument;
    class MediaManager;
    class Pattern;

    typedef Monitor PlaybackMonitor;

    /** Session, the main interface to the Audio Engine */
    class Sequence :  public ObjectModel,
                      public ValueTree::Listener,
                      public Timer
    {
    public:
        class Track : public TrackModel
        {
        public:

            Track (Sequence* s, const ValueTree& d)
                : TrackModel (d),
                  session (s)
            { }

            Track (const Track& other)
                : TrackModel (other.trackData),
                  session (other.session)
            { }

            virtual ~Track() { }

            String mediaType() const { return trackData.getProperty ("type", "invalid"); }

            String getName() const { return trackData.getProperty (Slugs::name, "invalid"); }
            Value getNameValue() { return trackData.getPropertyAsValue (Slugs::name, undoManager()); }
            void setName (const String &name) { getNameValue() = name; }

            inline int32 index()    const { return trackData.getParent().indexOf (trackData); }

            inline bool isAudio()   const { return trackData.getProperty ("type") == "audio"; }
            inline bool isMidi()    const { return trackData.getProperty ("type") == "midi"; }
            inline bool isPattern() const { return trackData.getProperty ("type") == "pattern"; }
            inline bool isValid()   const { return TrackModel::isValid() && mediaType() != "invalid"; }

            Track next() const;
            Track previous() const;

            inline int numClips() const { return trackData.getNumChildren(); }
            void removeFromSession();

            bool supportsAsset (const AssetItem& asset) const;
            bool supportsClip (const ClipModel& clip) const;
            bool supportsFile (const File& file) const;

            ClipModel addClip (const File& file, double startSeconds = 0.0f);

            inline ClipModel
            testAddClip (double time)
            {
                ClipModel clip (time, 1.0f);
                trackData.addChild (clip.node(), -1, undoManager());

                return clip;
            }

            inline void testPrint() const { std::clog << trackData.toXmlString(); }

        protected:

            Track (Sequence* s, const String& trackType)
                : TrackModel (Slugs::track),
                  session(s)
            {
                trackData.setProperty ("type", trackType, nullptr);
            }

            virtual bool canAccept (const AssetItem& asset) { return true; }

        private:

            inline Track& operator= (const Track& o) {
                session = o.session;
                trackData = o.trackData;
                return *this;
            }

            friend class Sequence;
            Sequence* session;

            ClipModel createClip() { return ClipModel (ValueTree::invalid); }
            UndoManager* undoManager() const { return nullptr; }

        };

        virtual ~Sequence();

        bool loadData (const ValueTree& data);

        void clear();
        void clearTracks();

        void open();
        void close();

        /** Get Monitors */
        void getMonitors (const Array<int>& path, Array<Shared<Monitor> >& monitors);
        void getMonitors (const ObjectModel& object, Array<Shared<Monitor> >& monitors);
        Shared<PlaybackMonitor> playbackMonitor();

        Shared<EngineControl> controller();
        Globals& globals();
        MediaManager& media();

        inline String name() const { return node().getProperty (Slugs::name, "Invalid Session"); }
        inline Value namevalue() { return getPropertyAsValue (Slugs::name); }
        inline void setName (const String& name) { setProperty (Slugs::name, name); }

        void appendTrack (const String& mediaType = "pattern");
        Track getTrack (int index);
        int numTracks() const;

        /** @internal  Some testing methods until the model-to-engine framework is solid */
        void testPrintXml();
        void testSetPlaying (bool isPlaying);
        void testSetRecording (bool isRecording);
        void testSetTempo (double tempo);

        /** @internal */
        void timerCallback();

        inline UndoManager* undoManager() { return nullptr; }

        XmlElement* createXml();

    protected:
        Sequence (Globals&);
        friend class Globals;

        /** Get the value tree that contains the sequence/tracks/clips */
        ValueTree sequenceNode() const;

        /** Get a track's value tree from the sequence */
        ValueTree trackNode (int trackIndex) const;

        /** Returns the the number of SessionRef's in use */
        inline int usageCount() const { return refs.load(); }

        /** Set a property.  This is a wrapper around the internal ValueTree */
        inline void setProperty (const Identifier& prop, const var& val) { node().setProperty (prop, val, nullptr); }

        friend class ValueTree;
        ValueTree projectState;
        virtual void valueTreePropertyChanged (ValueTree& treeWhosePropertyHasChanged, const Identifier& property);
        virtual void valueTreeChildAdded (ValueTree& parentTree, ValueTree& childWhichHasBeenAdded);
        virtual void valueTreeChildRemoved (ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int);
        virtual void valueTreeChildOrderChanged (ValueTree& parentTreeWhoseChildrenHaveMoved, int, int);
        virtual void valueTreeParentChanged (ValueTree& treeWhoseParentHasChanged);
        virtual void valueTreeRedirected (ValueTree& treeWhichHasBeenChanged);

    private:
        Globals& owner;

        class Private;
        ScopedPointer<Private> priv;

        void polishXml (XmlElement& e);

        void setMissingProperties (bool resetExisting = false);

        std::atomic<int> refs;
        friend void intrusive_ptr_add_ref (Session*);
        friend void intrusive_ptr_release (Session*);
        
        void notifyChanged() { }
    };
}

#endif // EL_SEQUENCE_H
