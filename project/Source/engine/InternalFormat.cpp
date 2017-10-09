/*
    InternalFormat.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#include "ElementApp.h"
#include "engine/AudioEngine.h"
#include "engine/GraphProcessor.h"
#include "engine/InternalFormat.h"
#include "engine/MidiSequenceProcessor.h"
#include "session/Session.h"
#include "Globals.h"

namespace Element {
    typedef GraphProcessor::AudioGraphIOProcessor IOP;
    
    InternalFormat::InternalFormat (AudioEngine& e)
        : engine (e)
    {
        {
            IOP p (IOP::audioOutputNode);
            p.fillInPluginDescription (audioOutDesc);
        }

        {
            IOP p (IOP::audioInputNode);
            p.fillInPluginDescription (audioInDesc);
        }

        {
            IOP p (IOP::midiOutputNode);
            p.fillInPluginDescription (midiOutDesc);
        }
        
        {
            IOP p (IOP::midiInputNode);
            p.fillInPluginDescription (midiInDesc);
        }
        
        {
            MidiSequenceProcessor p (engine);
            p.fillInPluginDescription (metroDesc);
        }
    }

    AudioPluginInstance* InternalFormat::instantiatePlugin (const PluginDescription& desc, double, int)
    {
        Globals& g (engine.globals());
        SessionRef s (g.getSession().makeRef());

        if (desc.fileOrIdentifier == audioOutDesc.fileOrIdentifier)
        {
            return new IOP (IOP::audioOutputNode);
        }
        else if (desc.fileOrIdentifier == audioInDesc.fileOrIdentifier)
        {
            return new IOP (IOP::audioInputNode);
        }
        else if (desc.fileOrIdentifier == midiInDesc.fileOrIdentifier)
        {
            return new IOP (IOP::midiInputNode);
        }
        else if (desc.fileOrIdentifier == midiOutDesc.fileOrIdentifier)
        {
            return new IOP (IOP::midiOutputNode);
        }
        else if (desc.name == samplerDesc.name)
        {
            return nullptr;
        }
        else if (desc.fileOrIdentifier == metroDesc.fileOrIdentifier)
        {
            return new MidiSequenceProcessor (engine);
        }
        else if (desc.name == "Sequencer")
        {
            return nullptr;
        }

        return nullptr;
    }

    const PluginDescription* InternalFormat::description (const InternalFormat::ID type)
    {
        switch (type)
        {
            case audioInputDevice:      return &audioInDesc;
            case audioOutputDevice:     return &audioOutDesc;
            case midiInputDevice:       return &midiInDesc;
            case midiOutputDevice:      return &midiOutDesc;
            case samplerProcessor:      return &samplerDesc;
            case sequenceProcessor:     return &sequencerDesc;
            case patternProcessor:      return &patternDesc;
            case midiSequence:          return &metroDesc;
            default:                    break;
        }

        return nullptr;
    }

    void InternalFormat::getAllTypes (OwnedArray <PluginDescription>& results)
    {
        for (int i = 0; i < (int) audioOutputPort; ++i)
            results.add (new PluginDescription (*description ((InternalFormat::ID) i)));
    }
    
    void InternalFormat::createPluginInstance (const PluginDescription& d, double initialSampleRate,
                                               int initialBufferSize, void* userData,
                                               void (*callback) (void*, AudioPluginInstance*, const String&))
    {
        if (auto* i = instantiatePlugin (d, initialSampleRate, initialBufferSize))
            callback (userData, i, String::empty);
    }
    
    bool InternalFormat::requiresUnblockedMessageThreadDuringCreation (const PluginDescription&) const noexcept
    {
        return false;
    }
}
