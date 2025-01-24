// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/node.h>
#include <element/juce/audio_processors.hpp>
#include <element/nodefactory.hpp>

namespace element {

class Context;

class ElementAudioPluginFormat : public juce::AudioPluginFormat
{
public:
    ElementAudioPluginFormat (Context&);
    ~ElementAudioPluginFormat() {}

    juce::String getName() const override { return EL_NODE_FORMAT_NAME; }
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
    Context& _context;
    juce::AudioPluginInstance* instantiatePlugin (const juce::PluginDescription& desc, double rate, int block);
};

class InternalNodes : public NodeProvider
{
public:
    InternalNodes() = delete;
    InternalNodes (Context& c);

    juce::String format() const override { return EL_NODE_FORMAT_NAME; }
    Processor* create (const String& ID) override;
    juce::StringArray findTypes (const juce::FileSearchPath&, bool, bool) override;
    juce::StringArray getHiddenTypes() override;

private:
    Context& _context;
};

} // namespace element
