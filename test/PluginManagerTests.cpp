// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include <element/context.hpp>
#include <element/plugins.hpp>
#include <element/settings.hpp>

#include "engine/clapprovider.hpp"
#include "utils.hpp"

using namespace element;

namespace {

/** Returns true if the unverified CLAP list contains the given file path. */
static bool unverifiedContains (PluginManager& manager, const juce::String& path)
{
    juce::OwnedArray<juce::PluginDescription> plugs;
    manager.getUnverifiedPlugins ("CLAP", plugs);
    for (const auto* const d : plugs)
        if (d->fileOrIdentifier == path)
            return true;
    return false;
}

} // namespace

BOOST_AUTO_TEST_SUITE (PluginManagerTests)

BOOST_AUTO_TEST_CASE (SupportedFormats)
{
    PluginManager manager;
    for (const auto& supported : Util::compiledAudioPluginFormats())
        BOOST_REQUIRE (! manager.isAudioPluginFormatSupported (supported));

    manager.getNodeFactory().add (new CLAPProvider());
    manager.addDefaultFormats();
    for (const auto& supported : Util::compiledAudioPluginFormats())
        BOOST_REQUIRE_MESSAGE (manager.isAudioPluginFormatSupported (supported), supported.toStdString());
}

#if ! JUCE_MAC
BOOST_AUTO_TEST_CASE (UnverifiedClapPlugins)
{
    const auto tempDir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                             .getChildFile ("element-unverified-claps");
    tempDir.deleteRecursively();
    BOOST_REQUIRE (tempDir.createDirectory().wasOk());
    const auto fakeClap = tempDir.getChildFile ("Fake.clap");
    BOOST_REQUIRE (fakeClap.create().wasOk());

    const auto propsFile = tempDir.getChildFile ("test.properties");
    juce::PropertiesFile::Options opts;
    opts.storageFormat = juce::PropertiesFile::storeAsXML;
    juce::PropertiesFile props (propsFile, opts);
    props.setValue (juce::String (Settings::lastPluginScanPathPrefix) + "CLAP",
                    tempDir.getFullPathName());

    PluginManager manager;
    manager.getNodeFactory().add (new CLAPProvider());
    manager.addDefaultFormats();
    manager.setPropertiesFile (&props);
    manager.searchUnverifiedPlugins();

    bool found = false;
    for (int retry = 0; ! found && retry < 250; ++retry)
    {
        found = unverifiedContains (manager, fakeClap.getFullPathName());
        if (! found)
            juce::Thread::sleep (20);
    }
    BOOST_REQUIRE_MESSAGE (found, "unverified scan should find the CLAP file");

    // Once known with an "id:path" identifier, it is no longer unverified.
    juce::PluginDescription desc;
    desc.pluginFormatName = "CLAP";
    desc.name = "Fake";
    desc.fileOrIdentifier = "com.fake.id:" + fakeClap.getFullPathName();
    manager.getKnownPlugins().addType (desc);
    BOOST_REQUIRE (! unverifiedContains (manager, fakeClap.getFullPathName()));

    manager.setPropertiesFile (nullptr);
    props.setNeedsToBeSaved (false);
    tempDir.deleteRecursively();
}
#endif

BOOST_AUTO_TEST_SUITE_END()
