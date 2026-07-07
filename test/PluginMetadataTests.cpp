// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include <element/datapath.hpp>
#include <element/plugins.hpp>

using namespace element;
using namespace juce;

namespace {

/** Backs up and restores the real plugin-metadata.xml around each test so the
    developer's actual plugin curation is never clobbered. */
struct MetadataFileGuard
{
    const File file { DataPath::applicationDataDir().getChildFile ("plugin-metadata.xml") };
    String backup;
    bool existed { false };

    MetadataFileGuard()
    {
        existed = file.existsAsFile();
        if (existed)
            backup = file.loadFileAsString();
        file.deleteFile();
    }

    ~MetadataFileGuard()
    {
        if (existed)
            file.replaceWithText (backup);
        else
            file.deleteFile();
    }
};

PluginDescription makeDesc (const String& name, const String& fileOrId, int uid)
{
    PluginDescription d;
    d.name = name;
    d.pluginFormatName = "VST3";
    d.fileOrIdentifier = fileOrId;
    d.uniqueId = uid;
    d.deprecatedUid = uid;
    return d;
}

} // namespace

BOOST_AUTO_TEST_SUITE (PluginMetadataTests)

BOOST_AUTO_TEST_CASE (HiddenExcludedFromVisible)
{
    MetadataFileGuard guard;
    PluginManager manager;

    const auto a = makeDesc ("Alpha", "/plugins/a.vst3", 1);
    const auto b = makeDesc ("Beta", "/plugins/b.vst3", 2);
    manager.getKnownPlugins().addType (a);
    manager.getKnownPlugins().addType (b);

    BOOST_REQUIRE_EQUAL (manager.getVisiblePluginTypes().size(), 2);

    manager.setPluginHidden (a, true);
    BOOST_REQUIRE (manager.isPluginHidden (a));

    auto visible = manager.getVisiblePluginTypes();
    BOOST_REQUIRE_EQUAL (visible.size(), 1);
    BOOST_REQUIRE (visible.getReference (0).createIdentifierString() == b.createIdentifierString());

    // The known list itself must be untouched (this is what makes it rescan-safe).
    BOOST_REQUIRE_EQUAL (manager.getKnownPlugins().getNumTypes(), 2);

    // Un-hiding restores visibility.
    manager.setPluginHidden (a, false);
    BOOST_REQUIRE (! manager.isPluginHidden (a));
    BOOST_REQUIRE_EQUAL (manager.getVisiblePluginTypes().size(), 2);
}

BOOST_AUTO_TEST_CASE (FavoritesFilter)
{
    MetadataFileGuard guard;
    PluginManager manager;

    const auto a = makeDesc ("Alpha", "/plugins/a.vst3", 1);
    const auto b = makeDesc ("Beta", "/plugins/b.vst3", 2);
    manager.getKnownPlugins().addType (a);
    manager.getKnownPlugins().addType (b);

    manager.setPluginFavorite (a, true);
    BOOST_REQUIRE (manager.isPluginFavorite (a));

    auto favorites = manager.getFavoritePluginTypes();
    BOOST_REQUIRE_EQUAL (favorites.size(), 1);
    BOOST_REQUIRE (favorites.getReference (0).createIdentifierString() == a.createIdentifierString());

    // A favorite that is also hidden should not appear in the favorites list.
    manager.setPluginHidden (a, true);
    BOOST_REQUIRE (manager.getFavoritePluginTypes().isEmpty());
}

BOOST_AUTO_TEST_CASE (SurvivesRescan)
{
    MetadataFileGuard guard;

    const auto a = makeDesc ("Alpha", "/plugins/a.vst3", 1);

    // Curate with one manager (persists to disk immediately).
    {
        PluginManager manager;
        manager.getKnownPlugins().addType (a);
        manager.setPluginHidden (a, true);
        manager.setPluginFavorite (a, true);
    }

    // A fresh manager (mimicking an app restart / rescan) restores the curation.
    PluginManager manager;
    XmlElement knownPlugins ("KNOWNPLUGINS");
    manager.restoreUserPlugins (knownPlugins);

    BOOST_REQUIRE (manager.isPluginHidden (a));
    BOOST_REQUIRE (manager.isPluginFavorite (a));
}

BOOST_AUTO_TEST_SUITE_END()
