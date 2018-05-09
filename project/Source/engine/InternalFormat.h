/*
    InternalFormat.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"

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
        void createPluginInstance (const PluginDescription&, double initialSampleRate, int initialBufferSize, void* userData,
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
        PluginDescription placeholderDesc;
        PluginDescription midiInputDeviceDesc;
        PluginDescription midiOutputDeviceDesc;

        AudioPluginInstance* instantiatePlugin (const PluginDescription& desc, double rate, int block);
    };

    class ElementAudioPluginFormat   : public AudioPluginFormat
    {
    public:
        ElementAudioPluginFormat (Globals&);
        ~ElementAudioPluginFormat() { }
        
        // AudioPluginFormat
        String getName() const                                      override { return "Element"; }
        bool fileMightContainThisPluginType (const String&)         override { return true; }
        FileSearchPath getDefaultLocationsToSearch()                override { return FileSearchPath(); }
        bool canScanForPlugins() const                              override { return true; }
        void findAllTypesForFile (OwnedArray <PluginDescription>& ds, const String&) override;
        bool doesPluginStillExist (const PluginDescription&)        override { return true; }
        String getNameOfPluginFromIdentifier (const String& fileOrIdentifier) override { return fileOrIdentifier; }
        bool pluginNeedsRescanning (const PluginDescription&)       override { return false; }
        StringArray searchPathsForPlugins (const FileSearchPath&, bool /*recursive*/, bool /*allowAsync*/) override;
        
    protected:
        void createPluginInstance (const PluginDescription&, double initialSampleRate, int initialBufferSize, void* userData,
                                   void (*callback) (void*, AudioPluginInstance*, const String&)) override;
        bool requiresUnblockedMessageThreadDuringCreation (const PluginDescription&) const noexcept override;
        
    private:
        Globals& world;
        PluginDescription reverbDesc;
        PluginDescription combFilterDesc;
        PluginDescription allPassFilterDesc;
        AudioPluginInstance* instantiatePlugin (const PluginDescription& desc, double rate, int block);
    };
}
