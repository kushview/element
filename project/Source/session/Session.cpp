/*
    Session.cpp - This file is part of Element
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


#include "engine/AudioEngine.h"
#include "engine/InternalFormat.h"
#include "engine/Transport.h"
#include "session/SessionAssets.h"
#include "session/Session.h"
#include "EngineControl.h"
#include "MediaManager.h"
#include "Globals.h"


namespace Element {
    class Session::Private
    {
    public:
        Private (Session& s, Globals& g)
            : session (s)
        {
            engine = g.engine();
            assert (engine != nullptr);
            playMonitor = engine->transport()->monitor();
            graph = engine->controller();
            assert (graph != nullptr);

            assets = new SessionAssets (session);
        }

        ~Private() { }

        void buildObjectMap() { }

        void clearSessionMedia()
        {
            assets->clear();
            session.media().clear();
        }

        void removeBogusTracks()
        {
            Array<int> removal;
            for (int track = session.numTracks(); --track >= 0;)
                if (! session.getTrack (track).isValid())
                    removal.add (track);

            for (int i = 0; i < removal.size(); ++i)
                session.sequenceNode().removeChild (i, nullptr);
        }

        void connectPattern (const AssetTree::Item& item) { }

        void setMissingProperties() {
        }

    private:
        friend class Session;
        Session&                     session;
        ScopedPointer<SessionAssets> assets;

        AudioEnginePtr          engine;
        Shared<EngineControl>   graph;
        Shared<PlaybackMonitor> playMonitor;
    };

    Session::Session (Globals& g)
        : ObjectModel ("session"),
          owner (g)
    {
        refs.store (0);

        setMissingProperties (true);
        priv = new Private (*this, g);
        projectState = node();
        projectState.addListener (this);

        startTimer (63);
    }

    Session::~Session()
    {
        projectState.removeListener (this);
        clear();

        priv = nullptr;

        if (refs.load() > 0) {
            jassertfalse;
            std::clog << "Session Destroyed with " << refs.load() << " references still existing\n";
        } else {
            std::clog << "Session Destroyed with no references\n";
        }
    }

    void
    Session::appendTrack (const String& mediaType)
    {
        const int index = numTracks();
        const Track end (getTrack (index - 1));

        ValueTree seq (sequenceNode());
        ValueTree td (Slugs::track);

        const String type = mediaType.isNotEmpty() ? mediaType : end.isValid() ? end.mediaType() : "pattern";

        td.setProperty ("type", type, nullptr);
        td.setProperty (Slugs::name, String("Track ") + String (numTracks()), nullptr);

        Track init (this, td);
        init.setMissingProperties();
        seq.addChild (init.trackData, -1, nullptr);

        assert (index == numTracks() - 1);
    }

    AssetTree& Session::assets() { assert (priv->assets); return *priv->assets; }

    int Session::numTracks() const
    {
        ValueTree seq (sequenceNode());
        return seq.isValid() ? seq.getNumChildren() : 0;
    }

    Session::Track Session::getTrack (int index)
    {
        ValueTree track (ValueTree::invalid);

        if (isPositiveAndBelow (index, numTracks()))
            track = trackNode (index);

        return Track (this, track);
    }

    Shared<PlaybackMonitor> Session::playbackMonitor() { return priv->playMonitor; }

    void Session::clear()
    {
        clearTracks();
        close();
        priv->clearSessionMedia();
        setMissingProperties (true);
    }

    void Session::clearTracks()
    {
        sequenceNode().removeAllChildren (undoManager());
    }

    void Session::close()
    {
        Track t (getTrack (0));
        while (t.isValid()) {
            t.state().removeProperty ("node", nullptr);
            t = t.next();
        }

        controller()->close();
    }

    Shared<EngineControl> Session::controller()
    {
        return priv->graph;
    }

    MediaManager& Session::media()
    {
        return globals().media();
    }


    Globals& Session::globals()
    {
        return owner;
    }


    bool
    Session::loadData (const ValueTree &data)
    {
        clear();
        projectState = ObjectModel::setData (data);

        ValueTree nd = node().getOrCreateChildWithName ("assets", nullptr);
        assets().setAssetsNode (nd);
        boost::function<void(const AssetItem&)> assetAdded =
                boost::bind (&SessionAssets::assetAdded, priv->assets.get(), ::_1);
        priv->assets->root().foreachChild (assetAdded);
        priv->removeBogusTracks();

        open();
        return true;
    }

    XmlElement* Session::createXml()
    {
        XmlElement* e = node().createXml();

        if (nullptr != e)
            polishXml (*e);

        return e;
    }

    void Session::open()
    {
        close();

        Shared<EngineControl> c (controller());
        c->setTempo (node().getProperty (Slugs::tempo, 120.0f));

        if (! c->open (*this)) {
            Logger::writeToLog ("SESSION: could not open the graph engine");
        }

    }

    void Session::testSetTempo (double tempo)
    {
        setProperty ("tempo", tempo);
        Shared<EngineControl> c (controller());
        c->setTempo (node().getProperty ("tempo", 120.0f));
    }

    void Session::testSetPlaying (bool isPlaying)
    {
        Shared<EngineControl> c (controller());
        c->setPlaying (isPlaying);
    }

    void Session::testSetRecording (bool isRecording)
    {
        Shared<EngineControl> c (controller());
        c->setRecording (isRecording);
    }

    void Session::testPrintXml()
    {
        std::clog << node().toXmlString() << std::endl;
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
            setProperty (Slugs::name, "Untited Session");

        if (! node().hasProperty ("tempo") || resetExisting)
            setProperty ("tempo", (double) 120.f);

        ValueTree seq = node().getOrCreateChildWithName ("sequence", nullptr);

        if (resetExisting)
            seq.removeAllChildren (nullptr);
    }

    void Session::timerCallback()
    {

    }

    void Session::valueTreePropertyChanged (ValueTree& tree, const Identifier& property)
    {
        if (tree != projectState)
            return;

        if (property == Slugs::tempo)
        {
            controller()->setTempo ((double) node().getProperty (property, 120.0f));
            notifyChanged();
        }
    }

    void Session::valueTreeChildAdded (ValueTree& parentTree, ValueTree& child)
    {
        notifyChanged();
    }

    void Session::valueTreeChildRemoved (ValueTree& parentTree, ValueTree& child, int)
    {
        notifyChanged();
    }

    void Session::valueTreeChildOrderChanged (ValueTree& parent, int, int)
    {
        notifyChanged();
    }

    void Session::valueTreeParentChanged (ValueTree& tree)
    {
        notifyChanged();
    }

    void Session::valueTreeRedirected (ValueTree& tree)
    {
        if (tree != projectState)
            return;

        Logger::writeToLog ("Session redirected");
        notifyChanged();
    }

    SequenceModel Session::sequence() const {
        SequenceModel model (sequenceNode());
        return model;
    }

    ValueTree Session::sequenceNode() const
    {
        return node().getChildWithName (Slugs::sequence);
    }


    ValueTree Session::trackNode (int trackIndex) const
    {
        if (isPositiveAndBelow (trackIndex, numTracks()))
            return sequenceNode().getChild (trackIndex);

        return ValueTree::invalid;
    }

    void intrusive_ptr_add_ref (Session * p)
    {
        ++p->refs;
    }

    void intrusive_ptr_release (Session * p)
    {
        if (p->refs > 0) {
            if (--p->refs == 0) {

            }
        }
    }


}
