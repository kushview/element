/*
    Session.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#include "engine/AudioEngine.h"
#include "engine/InternalFormat.h"
#include "engine/Transport.h"
#include "session/Session.h"
#include "MediaManager.h"
#include "Globals.h"

namespace Element {
    class Session::Private
    {
    public:
        Private (Session& s)
            : session (s)
        { }

        ~Private() { }
    private:
        friend class Session;
        Session&                     session;
    };

    Session::Session()
        : ObjectModel (Tags::session)
    {
        priv = new Private (*this);
        setMissingProperties (true);
        objectData.addListener (this);
    }

    Session::~Session()
    {
        objectData.removeListener (this);
        clear();

        priv = nullptr;

        if (getReferenceCount() > 0) {
            std::clog << "Session Destroyed with " << getReferenceCount() << " references still existing\n";
        } else {
            std::clog << "Session Destroyed with no references\n";
        }
    }
    
    void Session::clear()
    {
        setMissingProperties (true);
    }


    bool Session::loadData (const ValueTree &data)
    {
        clear();
        return true;
    }

    XmlElement* Session::createXml()
    {
        ValueTree saveData = node().createCopy();
        const ValueTree d = saveData.getChildWithName (Slugs::graph);
        if (d.isValid())
            saveData.removeChild (d, nullptr);
        
        XmlElement* e = saveData.createXml();
        if (nullptr != e)
            polishXml (*e);

        return e;
    }

    void Session::polishXml (XmlElement &e)
    {
        // remove generated properties from the engine
        const char* sa[] = { "node", "block", nullptr };
        StringArray trackAtts (sa);

        forEachXmlChildElementWithTagName (e, s, "sequence")
            forEachXmlChildElementWithTagName (*s, t, "track")
                for (const auto& a : trackAtts)
                    t->removeAttribute (a);
    }

    void Session::setMissingProperties (bool resetExisting)
    {
        if (! node().hasProperty (Slugs::name) || resetExisting)
            setProperty (Slugs::name, "Untitled Session");
        if (! node().hasProperty (Slugs::tempo) || resetExisting)
            setProperty (Slugs::tempo, (double) 120.f);
        objectData.getOrCreateChildWithName (Tags::graphs, nullptr);
    }

    void Session::valueTreePropertyChanged (ValueTree& tree, const Identifier& property)
    {
        sendChangeMessage();
    }

    void Session::valueTreeChildAdded (ValueTree& parentTree, ValueTree& child)
    {
        sendChangeMessage();
    }

    void Session::valueTreeChildRemoved (ValueTree& parentTree, ValueTree& child, int)
    {
        sendChangeMessage();
    }

    void Session::valueTreeChildOrderChanged (ValueTree& parent, int, int)
    {
        sendChangeMessage();
    }

    void Session::valueTreeParentChanged (ValueTree& tree)
    {
        sendChangeMessage();
    }

    void Session::valueTreeRedirected (ValueTree& tree)
    {
        sendChangeMessage();
    }
}
