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

#include <element/juce/audio_processors.hpp>

namespace element {

class Context;

/** Manages the internal plugin types. */
class InternalFormat : public juce::AudioPluginFormat
{
public:
    InternalFormat (Context&);
    ~InternalFormat() {}

    void getAllTypes (juce::OwnedArray<juce::PluginDescription>& results);

    // AudioPluginFormat
    juce::String getName() const override { return "Internal"; }
    bool fileMightContainThisPluginType (const juce::String&) override { return true; }
    juce::FileSearchPath getDefaultLocationsToSearch() override { return {}; }
    bool canScanForPlugins() const override { return false; }
    void findAllTypesForFile (juce::OwnedArray<juce::PluginDescription>& ds, const juce::String&) override {}
    bool doesPluginStillExist (const juce::PluginDescription&) override { return true; }
    juce::String getNameOfPluginFromIdentifier (const juce::String& fileOrIdentifier) override { return fileOrIdentifier; }
    bool pluginNeedsRescanning (const juce::PluginDescription&) override { return false; }
    juce::StringArray searchPathsForPlugins (const juce::FileSearchPath&, bool /*recursive*/, bool /*allowAsync*/) override { return juce::StringArray(); }
    bool isTrivialToScan() const override { return true; }

protected:
    void createPluginInstance (const juce::PluginDescription&,
                               double initialSampleRate,
                               int initialBufferSize,
                               PluginCreationCallback) override;
    bool requiresUnblockedMessageThreadDuringCreation (const juce::PluginDescription&) const noexcept override;

private:
    Context& context;
    juce::PluginDescription audioInDesc;
    juce::PluginDescription audioOutDesc;
    juce::PluginDescription midiInDesc;
    juce::PluginDescription midiOutDesc;
    juce::PluginDescription samplerDesc;
    juce::PluginDescription sequencerDesc;
    juce::PluginDescription patternDesc;
    juce::PluginDescription metroDesc;
    juce::PluginDescription placeholderDesc;
    juce::PluginDescription midiInputDeviceDesc;
    juce::PluginDescription midiOutputDeviceDesc;

    juce::AudioPluginInstance* instantiatePlugin (const juce::PluginDescription& desc, double rate, int block);
};

class ElementAudioPluginFormat : public juce::AudioPluginFormat
{
public:
    ElementAudioPluginFormat (Context&);
    ~ElementAudioPluginFormat() {}

    // AudioPluginFormat
    juce::String getName() const override { return "Element"; }
    bool fileMightContainThisPluginType (const juce::String&) override { return true; }
    juce::FileSearchPath getDefaultLocationsToSearch() override { return juce::FileSearchPath(); }
    bool canScanForPlugins() const override { return true; }
    void findAllTypesForFile (juce::OwnedArray<juce::PluginDescription>& ds, const juce::String&) override;
    bool doesPluginStillExist (const juce::PluginDescription&) override { return true; }
    juce::String getNameOfPluginFromIdentifier (const juce::String& fileOrIdentifier) override { return fileOrIdentifier; }
    bool pluginNeedsRescanning (const juce::PluginDescription&) override { return false; }
    juce::StringArray searchPathsForPlugins (const juce::FileSearchPath&, bool /*recursive*/, bool /*allowAsync*/) override;
    bool isTrivialToScan() const override { return true; }

protected:
    void createPluginInstance (const juce::PluginDescription&,
                               double initialSampleRate,
                               int initialBufferSize,
                               PluginCreationCallback) override;
    bool requiresUnblockedMessageThreadDuringCreation (const juce::PluginDescription&) const noexcept override;

private:
    Context& world;
    juce::PluginDescription reverbDesc;
    juce::PluginDescription combFilterDesc;
    juce::PluginDescription allPassFilterDesc;
    juce::AudioPluginInstance* instantiatePlugin (const juce::PluginDescription& desc, double rate, int block);
};

} // namespace element
