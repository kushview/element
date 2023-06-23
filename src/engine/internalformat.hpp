// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

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
