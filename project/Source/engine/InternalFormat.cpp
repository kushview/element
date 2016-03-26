/*
    InternalFormat.cpp - This file is part of Element
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

#include "element/Juce.h"
#include "session/Session.h"
#include "AudioEngine.h"
#include "InternalFormat.h"
#include "Globals.h"

namespace Element {

    InternalFormat::InternalFormat (AudioEngine& e)
        : engine (e)
    {
        {
            GraphProcessor::AudioGraphIOProcessor p (GraphProcessor::AudioGraphIOProcessor::audioOutputNode);
            p.fillInPluginDescription (audioOutDesc);
        }

        {
            GraphProcessor::AudioGraphIOProcessor p (GraphProcessor::AudioGraphIOProcessor::audioInputNode);
            p.fillInPluginDescription (audioInDesc);
        }

        {
            GraphProcessor::AudioGraphIOProcessor p (GraphProcessor::AudioGraphIOProcessor::midiInputNode);
            p.fillInPluginDescription (midiInDesc);
        }

        {
            GraphProcessor::AudioGraphIOProcessor p (GraphProcessor::AudioGraphIOProcessor::midiOutputNode);
            p.fillInPluginDescription (midiInDesc);
        }

        {
           // Element::SamplerProcessor p;
         //   p.fillInPluginDescription (samplerDesc);
        }

        {
            patternDesc.name = "Pattern";
        }

        {
            sequencerDesc.name = "Sequencer";
        }
    }

    AudioPluginInstance*
    InternalFormat::createInstanceFromDescription (const PluginDescription& desc, double, int)
    {
        Globals& g (engine.globals());
        SessionRef s (g.session().makeRef());

        if (desc.name == audioOutDesc.name)
        {
            return new GraphProcessor::AudioGraphIOProcessor (GraphProcessor::AudioGraphIOProcessor::audioOutputNode);
        }
        else if (desc.name == audioInDesc.name)
        {
            return new GraphProcessor::AudioGraphIOProcessor (GraphProcessor::AudioGraphIOProcessor::audioInputNode);
        }
        else if (desc.name == midiInDesc.name)
        {
            return new GraphProcessor::AudioGraphIOProcessor (GraphProcessor::AudioGraphIOProcessor::midiInputNode);
        }
        else if (desc.name == midiOutDesc.name)
        {
            return new GraphProcessor::AudioGraphIOProcessor (GraphProcessor::AudioGraphIOProcessor::midiOutputNode);
        }
        else if (desc.name == samplerDesc.name)
        {
            // return new BTSP1::PluginProcessor (engine.globals());
        }
        else if (desc.name == "Sequencer")
        {
            Sequencer* seq = new Sequencer (engine.clips());
            seq->setModel (s->sequence());

            if (! seq->model().node().isValid())
            {
                Logger::writeToLog ("Internal Format: created sequence is invalid");
                delete seq;
                seq = nullptr;
            }
            else
            {
                assert (seq->model().node() == s->sequence().node());
            }

            return seq;
        }

        return nullptr;
    }

    const PluginDescription*
    InternalFormat::description (const InternalFormat::ID type)
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
            default:                    break;
        }

        return nullptr;
    }

    void
    InternalFormat::getAllTypes (OwnedArray <PluginDescription>& results)
    {
        for (int i = 0; i < (int) audioOutputPort; ++i)
            results.add (new PluginDescription (*description ((InternalFormat::ID) i)));
    }

}
