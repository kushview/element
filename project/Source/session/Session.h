/*
    Session.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"

namespace Element {
    class AssetTree;
    class Session;
    class EngineControl;
    class Globals;
    class Instrument;
    class MediaManager;
    class Pattern;
    
    /** Session, the main interface between the engine and model layers */
    class Session : public ObjectModel,
                    public ReferenceCountedObject,
                    public ChangeBroadcaster,
                    public ValueTree::Listener
    {
    public:
        virtual ~Session();
        
        inline int getNumGraphs() const { return objectData.getChildWithName(Tags::graphs).getNumChildren(); }
        
        inline ValueTree getGraphValueTree (const int index) const { return getGraphsValueTree().getChild (index); }
        
        ValueTree getValueTree() const { return objectData; }
        bool loadData (const ValueTree& data);
        void clear();

        inline String getName() const { return node().getProperty(Slugs::name, "Invalid Session"); }
        inline Value getNamevalue() { return getPropertyAsValue (Slugs::name); }
        inline void setName (const String& name) { setProperty (Slugs::name, name); }

        XmlElement* createXml();
        
        void saveGraphState();
    protected:
        Session();
        friend class Globals;
 
        /** Set a property.  This is a wrapper around the internal ValueTree */
        inline void setProperty (const Identifier& prop, const var& val) { node().setProperty (prop, val, nullptr); }

        friend class ValueTree;
        virtual void valueTreePropertyChanged (ValueTree& treeWhosePropertyHasChanged, const Identifier& property);
        virtual void valueTreeChildAdded (ValueTree& parentTree, ValueTree& childWhichHasBeenAdded);
        virtual void valueTreeChildRemoved (ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int);
        virtual void valueTreeChildOrderChanged (ValueTree& parentTreeWhoseChildrenHaveMoved, int, int);
        virtual void valueTreeParentChanged (ValueTree& treeWhoseParentHasChanged);
        virtual void valueTreeRedirected (ValueTree& treeWhichHasBeenChanged);

    private:
        class Private;
        ScopedPointer<Private> priv;
        inline ValueTree getGraphsValueTree() const { return objectData.getChildWithName (Tags::graphs); }
        void polishXml (XmlElement& e);
        void setMissingProperties (bool resetExisting = false);
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Session);
    };
    
    typedef ReferenceCountedObjectPtr<Session> SessionPtr;
    typedef SessionPtr SessionRef;
}
