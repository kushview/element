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
        setMissingProperties (true);
        priv = new Private (*this);
    }

    Session::~Session()
    {
        projectState.removeListener (this);
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
        projectState = ObjectModel::setData (data);
        ValueTree nd = node().getOrCreateChildWithName ("assets", nullptr);
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
    }

    void Session::valueTreeChildAdded (ValueTree& parentTree, ValueTree& child)
    {
    }

    void Session::valueTreeChildRemoved (ValueTree& parentTree, ValueTree& child, int)
    {
    }

    void Session::valueTreeChildOrderChanged (ValueTree& parent, int, int)
    {
    }

    void Session::valueTreeParentChanged (ValueTree& tree)
    {
    }

    void Session::valueTreeRedirected (ValueTree& tree)
    {
    }
}
