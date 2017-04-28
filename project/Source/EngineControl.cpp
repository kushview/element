/*
    EngineControl.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#include "engine/AudioEngine.h"
#include "engine/Transport.h"
#include "session/PluginManager.h"
#include "EngineControl.h"
#include "Globals.h"

namespace Element {

    EngineControl::EngineControl (AudioEngine& e)
        : GraphController (e.graph(), e.globals().getPluginManager()),
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

    GraphController* EngineControl::createSequenceController()
    {
       #if 0
        // not supported. saved for reference.
        if (seqNode == nullptr) {
            seqNode = addRootPlugin (InternalFormat::sequenceProcessor);
        }

        if (sequencer() != nullptr)
            return new GraphController (*sequencer(), engine.globals().plugins());
       #endif
        return nullptr;
    }

    GraphNodePtr EngineControl::addRootPlugin (IOProcessor::IODeviceType ioType)
    {
        switch (ioType)
        {
            case IOProcessor::audioInputNode:  return addRootPlugin (InternalFormat::audioInputDevice);
            case IOProcessor::audioOutputNode: return addRootPlugin (InternalFormat::audioOutputDevice);
            case IOProcessor::midiInputNode:   return addRootPlugin (InternalFormat::midiInputDevice);
            case IOProcessor::midiOutputNode:  return addRootPlugin (InternalFormat::midiOutputDevice);
            default: break;
        }

        return GraphNodePtr();
    }

    GraphNodePtr
    EngineControl::addRootPlugin (InternalFormat::ID internal)
    {
        const uint32 id = addFilter (internals()->description (internal));
        return getNodeForId (id);
    }

    bool EngineControl::close()
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


    InternalFormat* EngineControl::internals() const
    {
        return engine.globals().getPluginManager().format<InternalFormat>();
    }

    bool EngineControl::open (Session &s)
    {
        jassert (session == nullptr);
        session = s.makeRef();

        for (int i = 0; i < getNumFilters(); ++i)
        {
            NodePtr node = getNode (i);
            if (IOProcessor* io = node->processor<IOProcessor>())
                ioNodes.set (io->getType(), node);
        }
#if 0
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
#endif

#if 0
        NodePtr output = ioNodes.getUnchecked (IOProcessor::audioOutputNode);
        

        getGraph().connectChannels (PortType::Audio, seqNode->nodeId, 0, output->nodeId, 0);
        getGraph().connectChannels (PortType::Audio, seqNode->nodeId, 1, output->nodeId, 1);
#endif
        
        session->node().addListener (this);
        return true;
    }

    void EngineControl::setPlaying (bool p)
    {
        engine.transport()->requestPlayState (p);
    }

    Sequencer* EngineControl::sequencer() const
    {
        return nullptr;
    }

    void EngineControl::setRecording (bool r)
    {
        engine.transport()->requestRecordState (r);
    }

    void EngineControl::setTempo (double newTempo)
    {
        engine.transport()->requestTempo (newTempo);
    }

    bool EngineControl::validateRequiredNodes()
    {
#if 0
        if (sequencer() == nullptr)
            return false;
#endif
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

    void EngineControl::valueTreePropertyChanged (ValueTree&, const Identifier&)
    {

    }

    void EngineControl::valueTreeChildAdded (ValueTree&, ValueTree&)
    {

    }

    void EngineControl::valueTreeChildRemoved (ValueTree&, ValueTree& , int)
    {

    }

    void EngineControl::valueTreeChildOrderChanged (ValueTree&, int, int)
    {

    }

    void EngineControl::valueTreeParentChanged (ValueTree&)
    {

    }

    void EngineControl::valueTreeRedirected (ValueTree&)
    {

    }
}
