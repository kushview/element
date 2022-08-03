/*
    This file is part of Element
    Copyright (C) 2014-2019  Kushview, LLC.  All rights reserved.

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
#include "session/Session.h"
#include "session/Sequence.h"
#include "MediaManager.h"
#include "Globals.h"

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wparentheses")

namespace Element {

class Sequence::Private
{
public:
    Private (Sequence& s, Globals& g)
        : session (s)
    {
        engine = g.getAudioEngine();
        jassert (engine != nullptr);
        playMonitor = nullptr;
    }

    ~Private() {}

    void buildObjectMap() {}

    void clearSessionMedia()
    {
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

    void connectPattern (const AssetTree::Item& item) {}

    void setMissingProperties()
    {
    }

private:
    friend class Sequence;
    Sequence& session;
    AudioEnginePtr engine;
    Shared<EngineControl> graph;
    Shared<PlaybackMonitor> playMonitor;
};

Sequence::Sequence (Globals& g)
    : ObjectModel (Slugs::sequence),
      owner (g)
{
    refs.store (0);

    setMissingProperties (true);
    priv = new Private (*this, g);
    projectState = node();
    projectState.addListener (this);

    startTimer (63);
}

Sequence::~Sequence()
{
    projectState.removeListener (this);
    clear();

    priv = nullptr;
}

void Sequence::appendTrack (const String& mediaType)
{
#if 0
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
#endif
}

int Sequence::numTracks() const
{
    ValueTree seq (sequenceNode());
    return seq.isValid() ? seq.getNumChildren() : 0;
}

Sequence::Track Sequence::getTrack (int index)
{
    ValueTree track;

    if (isPositiveAndBelow (index, numTracks()))
        track = trackNode (index);

    return Track (this, track);
}

Shared<PlaybackMonitor> Sequence::playbackMonitor() { return priv->playMonitor; }

void Sequence::clear()
{
    clearTracks();
    close();
    priv->clearSessionMedia();
    setMissingProperties (true);
}

void Sequence::clearTracks()
{
    sequenceNode().removeAllChildren (undoManager());
}

void Sequence::close()
{
    Track t (getTrack (0));
    while (t.isValid())
    {
        t.state().removeProperty ("node", nullptr);
        t = t.next();
    }
}

Shared<EngineControl> Sequence::controller()
{
    return priv->graph;
}

MediaManager& Sequence::media()
{
    return globals().getMediaManager();
}

Globals& Sequence::globals()
{
    return owner;
}

bool Sequence::loadData (const ValueTree& data)
{
#if 0
        clear();
        projectState = ObjectModel::setData (data);

        ValueTree nd = node().getOrCreateChildWithName ("assets", nullptr);
        assets().setAssetsNode (nd);
        boost::function<void(const AssetItem&)> assetAdded =
                boost::bind (&SessionAssets::assetAdded, priv->assets.get(), ::_1);
        priv->assets->root().foreachChild (assetAdded);
        priv->removeBogusTracks();

        open();
#endif
    return true;
}

std::unique_ptr<XmlElement> Sequence::createSequenceXml()
{
    auto e = getValueTree().createXml();
    if (nullptr != e)
        polishXml (*e);

    return e;
}

void Sequence::open()
{
    close();
#if 0
        Shared<EngineControl> c (controller());
        c->setTempo (node().getProperty (Slugs::tempo, 120.0f));

        if (! c->open (*this)) {
            Logger::writeToLog ("SESSION: could not open the graph engine");
        }
#endif
}

void Sequence::testSetTempo (double tempo)
{
    setProperty ("tempo", tempo);
}

void Sequence::testSetPlaying (bool isPlaying)
{
    Shared<EngineControl> c (controller());
}

void Sequence::testSetRecording (bool isRecording)
{
    Shared<EngineControl> c (controller());
}

void Sequence::testPrintXml()
{
    std::clog << node().toXmlString() << std::endl;
}

void Sequence::polishXml (XmlElement& e)
{
    // remove generated properties from the engine
    const char* sa[] = { "node", "block", nullptr };
    StringArray trackAtts (sa);

    forEachXmlChildElementWithTagName (e, s, "sequence")
        forEachXmlChildElementWithTagName (*s, t, "track") 
            for (const auto& a : trackAtts)
                t->removeAttribute (a);
}

void Sequence::setMissingProperties (bool resetExisting)
{
    if (! node().hasProperty (Slugs::name) || resetExisting)
        setProperty (Slugs::name, "Untited Session");

    if (! node().hasProperty ("tempo") || resetExisting)
        setProperty ("tempo", (double) 120.f);

    ValueTree graph = node().getOrCreateChildWithName (Slugs::graph, nullptr);
    ValueTree seq = node().getOrCreateChildWithName (Slugs::sequence, nullptr);

    if (resetExisting)
    {
        graph.removeAllChildren (nullptr);
        seq.removeAllChildren (nullptr);
    }
}

void Sequence::timerCallback()
{
}

void Sequence::valueTreePropertyChanged (ValueTree& tree, const Identifier& property)
{
    if (tree != projectState)
        return;

    if (property == Slugs::tempo)
    {
        notifyChanged();
    }
}

void Sequence::valueTreeChildAdded (ValueTree& parentTree, ValueTree& child)
{
    notifyChanged();
}

void Sequence::valueTreeChildRemoved (ValueTree& parentTree, ValueTree& child, int)
{
    notifyChanged();
}

void Sequence::valueTreeChildOrderChanged (ValueTree& parent, int, int)
{
    notifyChanged();
}

void Sequence::valueTreeParentChanged (ValueTree& tree)
{
    notifyChanged();
}

void Sequence::valueTreeRedirected (ValueTree& tree)
{
    if (tree != projectState)
        return;

    Logger::writeToLog ("Session redirected");
    notifyChanged();
}

ValueTree Sequence::sequenceNode() const
{
    return node().getChildWithName (Slugs::sequence);
}

ValueTree Sequence::trackNode (int trackIndex) const
{
    if (isPositiveAndBelow (trackIndex, numTracks()))
        return sequenceNode().getChild (trackIndex);

    return ValueTree();
}
} // namespace Element

JUCE_END_IGNORE_WARNINGS_GCC_LIKE
