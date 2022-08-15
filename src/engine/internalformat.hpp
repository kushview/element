/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

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

#pragma once

#include "ElementApp.h"

namespace element {

class AudioEngine;
class MidiEngine;
class Context;

/** Manages the internal plugin types. */
class InternalFormat : public AudioPluginFormat
{
public:
    InternalFormat (AudioEngine&, MidiEngine&);
    ~InternalFormat() {}

    void getAllTypes (OwnedArray<PluginDescription>& results);

    // AudioPluginFormat
    String getName() const override { return "Internal"; }
    bool fileMightContainThisPluginType (const String&) override { return true; }
    FileSearchPath getDefaultLocationsToSearch() override { return FileSearchPath(); }
    bool canScanForPlugins() const override { return false; }
    void findAllTypesForFile (OwnedArray<PluginDescription>& ds, const String&) override {}
    bool doesPluginStillExist (const PluginDescription&) override { return true; }
    String getNameOfPluginFromIdentifier (const String& fileOrIdentifier) override { return fileOrIdentifier; }
    bool pluginNeedsRescanning (const PluginDescription&) override { return false; }
    StringArray searchPathsForPlugins (const FileSearchPath&, bool /*recursive*/, bool /*allowAsync*/) override { return StringArray(); }
    bool isTrivialToScan() const override { return true; }

protected:
    void createPluginInstance (const PluginDescription&,
                               double initialSampleRate,
                               int initialBufferSize,
                               PluginCreationCallback) override;
    bool requiresUnblockedMessageThreadDuringCreation (const PluginDescription&) const noexcept override;

private:
    AudioEngine& engine;
    MidiEngine& midi;
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

class ElementAudioPluginFormat : public AudioPluginFormat
{
public:
    ElementAudioPluginFormat (Context&);
    ~ElementAudioPluginFormat() {}

    // AudioPluginFormat
    String getName() const override { return "Element"; }
    bool fileMightContainThisPluginType (const String&) override { return true; }
    FileSearchPath getDefaultLocationsToSearch() override { return FileSearchPath(); }
    bool canScanForPlugins() const override { return true; }
    void findAllTypesForFile (OwnedArray<PluginDescription>& ds, const String&) override;
    bool doesPluginStillExist (const PluginDescription&) override { return true; }
    String getNameOfPluginFromIdentifier (const String& fileOrIdentifier) override { return fileOrIdentifier; }
    bool pluginNeedsRescanning (const PluginDescription&) override { return false; }
    StringArray searchPathsForPlugins (const FileSearchPath&, bool /*recursive*/, bool /*allowAsync*/) override;
    bool isTrivialToScan() const override { return true; }

protected:
    void createPluginInstance (const PluginDescription&,
                               double initialSampleRate,
                               int initialBufferSize,
                               PluginCreationCallback) override;
    bool requiresUnblockedMessageThreadDuringCreation (const PluginDescription&) const noexcept override;

private:
    Context& world;
    PluginDescription reverbDesc;
    PluginDescription combFilterDesc;
    PluginDescription allPassFilterDesc;
    AudioPluginInstance* instantiatePlugin (const PluginDescription& desc, double rate, int block);
};

} // namespace element
