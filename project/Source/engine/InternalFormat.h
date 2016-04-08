/*
    InternalFormat.h - This file is part of Element
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

#ifndef ELEMENT_INTERNAL_FORMAT_H
#define ELEMENT_INTERNAL_FORMAT_H

#include "element/Juce.h"

namespace Element {

    class AudioEngine;
    class Globals;

    /** Manages the internal plugin types.
        @warning Usage of this format before AudioEngine is added to Globals
        will yield unexpected results */
    class InternalFormat   : public AudioPluginFormat
    {
    public:

        enum ID
        {
            audioInputDevice = 0,
            audioOutputDevice,
            midiInputDevice,
            midiOutputDevice,
            midiSequence,
            audioInputPort,
            audioOutputPort,
            atomInputPort,
            atomOutputPort,
            controlInputPort,
            controlOutputPort,
            cvOutPort,
            cvInPort,
            midiInputPort,
            midiOutputPort,
            metronomeProcessor,
            patternProcessor,
            samplerProcessor,
            sequenceProcessor,
            internalTypesEnd
        };

        InternalFormat (AudioEngine&);
        ~InternalFormat() { }

        const PluginDescription* description (const InternalFormat::ID type);
        void getAllTypes (OwnedArray <PluginDescription>& results);

        // AudioPluginFormat
        String getName() const                                      override { return "Internal"; }
        bool fileMightContainThisPluginType (const String&)         override { return true; }
        FileSearchPath getDefaultLocationsToSearch()                override { return FileSearchPath(); }
        bool canScanForPlugins() const                              override { return false; }
        void findAllTypesForFile (OwnedArray <PluginDescription>& ds, const String&) override { }
        bool doesPluginStillExist (const PluginDescription&)        override { return true; }
        String getNameOfPluginFromIdentifier (const String& fileOrIdentifier) override { return fileOrIdentifier; }
        bool pluginNeedsRescanning (const PluginDescription&) override { return false; }
        StringArray searchPathsForPlugins (const FileSearchPath&, bool /*recursive*/, bool /*allowAsync*/) override { return StringArray(); }
        

    protected:
        void createPluginInstance (const PluginDescription&, double initialSampleRate,
                                   int initialBufferSize, void* userData,
                                   void (*callback) (void*, AudioPluginInstance*, const String&)) override;
        bool requiresUnblockedMessageThreadDuringCreation (const PluginDescription&) const noexcept override;
    
    private:
        AudioEngine& engine;
        PluginDescription audioInDesc;
        PluginDescription audioOutDesc;
        PluginDescription midiInDesc;
        PluginDescription midiOutDesc;
        PluginDescription samplerDesc;
        PluginDescription sequencerDesc;
        PluginDescription patternDesc;
        PluginDescription metroDesc;
        
        AudioPluginInstance* instantiatePlugin (const PluginDescription& desc, double rate, int block);
    };
}

#endif   // ELEMENT_INTERNAL_FORMAT_H
