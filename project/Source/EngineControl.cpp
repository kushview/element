/*
    EngineControl.cpp - This file is part of Element
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

#include "engine/AudioEngine.h"
#include "engine/Transport.h"

#include "EngineControl.h"
#include "Globals.h"

namespace Element {

    EngineControl::EngineControl (AudioEngine& e)
        : GraphController (e.graph(), e.globals().plugins()),
          world (e.globals()), engine (e)
    {
        ioNodes.ensureStorageAllocated (IOProcessor::numDeviceTypes);
        while (ioNodes.size() < 5)
            ioNodes.add (nullptr);
    }

    EngineControl::~EngineControl()
    {
        ioNodes.clearQuick();
        seqNode = nullptr;
    }

    GraphController*
    EngineControl::createSequenceController()
    {
        if (seqNode == nullptr) {
            seqNode = addRootPlugin (InternalFormat::sequenceProcessor);
        }

        if (sequencer() != nullptr)
            return new GraphController (*sequencer(), engine.globals().plugins());

        return nullptr;
    }

    EngineControl::NodePtr
    EngineControl::addRootPlugin (IOProcessor::IODeviceType ioType)
    {
        switch (ioType)
        {
            case IOProcessor::audioInputNode:  return addRootPlugin (InternalFormat::audioInputDevice);
            case IOProcessor::audioOutputNode: return addRootPlugin (InternalFormat::audioOutputDevice);
            case IOProcessor::midiInputNode:   return addRootPlugin (InternalFormat::midiInputDevice);
            case IOProcessor::midiOutputNode:  return addRootPlugin (InternalFormat::midiOutputDevice);
            default: break;
        }

        return NodePtr ();
    }

    EngineControl::NodePtr
    EngineControl::addRootPlugin (InternalFormat::ID internal)
    {
        const uint32 id = addFilter (internals()->description (internal));
        return getNodeForId (id);
    }

    bool
    EngineControl::close()
    {
        if (session.get() == nullptr) {
            // haven't been opened or close has been called more than once
            return true;
        }

        session->node().removeListener (this);
        ioNodes.clearQuick();
        session.reset();
        return true;
    }


    InternalFormat*
    EngineControl::internals() const
    {
        return engine.globals().plugins().format<InternalFormat>();
    }

    bool
    EngineControl::open (Session &s)
    {
        jassert (session == nullptr);
        session = s.makeRef();

        for (int i = 0; i < getNumFilters(); ++i)
        {
            NodePtr node = getNode (i);
            if (IOProcessor* io = node->processor<IOProcessor>())
                ioNodes.set (io->getType(), node);
        }

        if (seqNode == nullptr) {
            seqNode = addRootPlugin (InternalFormat::sequenceProcessor);
        }

        Sequencer* seq = sequencer();

        if (! validateRequiredNodes()) {
            jassertfalse;
            Logger::writeToLog ("Engine Control: node validation failed");
            return false;
        }

        SequenceModel sm (session->sequence());
        seq->setModel (sm);

        for (int i = 0; i < session->numTracks(); ++i) {
            Session::Track track (session->getTrack (i));
            seq->assignTrack (i, track);
        }

        assert (seq->numTracks() == session->numTracks());
        assert (seq->tracksArePresent());

        NodePtr output = ioNodes.getUnchecked (IOProcessor::audioOutputNode);
        getGraph().connectChannels (PortType::Audio, seqNode->nodeId, 0, output->nodeId, 0);
        getGraph().connectChannels (PortType::Audio, seqNode->nodeId, 1, output->nodeId, 1);

        session->node().addListener (this);
        return true;
    }

    void
    EngineControl::setPlaying (bool p)
    {
        engine.transport()->requestPlayState (p);
    }

    Sequencer*
    EngineControl::sequencer() const
    {
        return seqNode != nullptr ? seqNode->processor<Sequencer>() : nullptr;
    }

    void
    EngineControl::setRecording (bool r)
    {
        engine.transport()->requestRecordState (r);
    }

    void
    EngineControl::setTempo (double newTempo)
    {
        engine.transport()->requestTempo (newTempo);
    }

    bool
    EngineControl::validateRequiredNodes()
    {
        if (sequencer() == nullptr)
            return false;

        for (int i = 0; i < IOProcessor::numDeviceTypes; ++i)
        {
            NodePtr ptr = ioNodes.getUnchecked (i);
            if (ptr) continue;

            ptr = addRootPlugin (static_cast<IOProcessor::IODeviceType> (i));

            if (ptr == nullptr)
                return false;

            ioNodes.set (i, ptr);
        }

        return true;
    }

    void
    EngineControl::valueTreePropertyChanged (ValueTree& /*tree*/, const Identifier& /*prop*/)
    {

    }

    void
    EngineControl::valueTreeChildAdded (ValueTree& parent, ValueTree& child)
    {

    }

    void
    EngineControl::valueTreeChildRemoved (ValueTree& /*parent*/, ValueTree& child)
    {

    }

    void
    EngineControl::valueTreeChildOrderChanged (ValueTree& parent)
    {

    }

    void
    EngineControl::valueTreeParentChanged (ValueTree& tree)
    {

    }

    void
    EngineControl::valueTreeRedirected (ValueTree& tree)
    {

    }

}
