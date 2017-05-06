/*
    Session.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#ifndef ELEMENT_SESSION_H
#define ELEMENT_SESSION_H

#include "ElementApp.h"
#include <boost/intrusive_ptr.hpp>

namespace Element {
    class AssetTree;
    class Session;
    class EngineControl;
    class Globals;
    class Instrument;
    class MediaManager;
    class Pattern;

    // typedef Monitor PlaybackMonitor;

    void intrusive_ptr_add_ref (Session * p);
    void intrusive_ptr_release (Session * p);


    /** Slim wrapper for a boost::intrusive_ptr with special qualities
        for Session

        Objects that aren't owned by Session but need to keep a long standing
        reference to it should utilize this class

        @see Session::makeRef()
    */
    class SessionRef : public boost::intrusive_ptr<Session>
    {
    public:
        explicit SessionRef (Session* s = nullptr)
            : boost::intrusive_ptr<Session> (s) { }
        
        SessionRef (const SessionRef& o) {
            boost::intrusive_ptr<Session>::operator= (o);
        }
    };

    /** Session, the main interface to the Audio Engine */
    class Session :  public ObjectModel,
                     public ValueTree::Listener,
                     public Timer
    {
    public:
        virtual ~Session();

        bool loadData (const ValueTree& data);

        void clear();
        void clearTracks();

        void open();
        void close();

        /** Get Monitors */
        void getMonitors (const Array<int>& path, Array<Shared<Monitor> >& monitors);
        void getMonitors (const ObjectModel& object, Array<Shared<Monitor> >& monitors);
        Shared<Monitor> getPlaybackMonitor();

        AssetTree& assets();
        
        Globals& globals();
        MediaManager& media();

        inline String name() const { return node().getProperty (Slugs::name, "Invalid Session"); }
        inline Value namevalue() { return getPropertyAsValue (Slugs::name); }
        inline void setName (const String& name) { setProperty (Slugs::name, name); }

        inline SessionRef makeRef() { SessionRef ref (this); return ref; }

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
        /** Create a new session object. Session is created once and owned by Globals
            @see Globals::setEngine */
        Session (Globals&);
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

#endif // ELEMENT_SESSION_H
