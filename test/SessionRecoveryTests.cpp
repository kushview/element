// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include <element/context.hpp>
#include <element/datapath.hpp>
#include <element/node.hpp>
#include <element/services.hpp>
#include <element/session.hpp>
#include <element/settings.hpp>
#include <element/tags.hpp>

#include "services/sessionservice.hpp"
#include "testutil.hpp"

using namespace element;
using namespace juce;
namespace et = element::test;

namespace {

SessionService& sessions()
{
    auto* svc = et::context()->services().find<SessionService>();
    BOOST_REQUIRE (svc != nullptr);
    return *svc;
}

void pump (int ms = 30)
{
    MessageManager::getInstance()->runDispatchLoopUntil (ms);
}

/** Adds a graph so the document becomes dirty. */
void dirtySession()
{
    et::context()->session()->addGraph (Node::createDefaultGraph ("Dirty"), true);
    pump();
    BOOST_REQUIRE (sessions().hasSessionChanged());
}

void renameActiveGraph (const String& name)
{
    auto graph = et::context()->session()->getActiveGraph();
    auto tree = graph.data();
    tree.setProperty (tags::name, name, nullptr);
    pump();
}

/** Leaves the shared context with a clean, untitled session and no sidecars. */
struct CleanState {
    CleanState() { reset(); }
    ~CleanState() { reset(); }

    void reset()
    {
        auto& svc = sessions();
        svc.resetChanges (true);
        svc.deleteRecoveryFile();
        SessionService::untitledRecoveryFile().deleteFile();
    }
};

struct TempSession {
    TempSession()
        : dir (File::createTempFile ("sessions"))
    {
        dir.deleteFile();
        dir.createDirectory();
        file = dir.getChildFile ("recover.els");
        auto xml = et::context()->session()->createXml();
        BOOST_REQUIRE (xml != nullptr);
        BOOST_REQUIRE (xml->writeTo (file));
    }

    ~TempSession() { dir.deleteRecursively(); }

    File dir, file;
};

/** The shared test context uses the developer's real settings file, and
    saving or recovering a session persists Settings::lastSessionKey. Snapshot
    it around each case so the suite leaves the setting as it found it. */
struct LastSessionSnapshot {
    LastSessionSnapshot()
    {
        auto* props = et::context()->settings().getUserSettings();
        had = props->containsKey (Settings::lastSessionKey);
        value = props->getValue (Settings::lastSessionKey);
    }

    ~LastSessionSnapshot()
    {
        auto* props = et::context()->settings().getUserSettings();
        if (had)
            props->setValue (Settings::lastSessionKey, value);
        else
            props->removeValue (Settings::lastSessionKey);
        props->saveIfNeeded();
    }

    bool had = false;
    String value;
};

} // namespace

BOOST_FIXTURE_TEST_SUITE (SessionRecoveryTests, LastSessionSnapshot)

BOOST_AUTO_TEST_CASE (RecoveryFileForNamedAndUntitled)
{
    const File named ("/a/b.els");
    BOOST_REQUIRE (SessionService::recoveryFileFor (named) == File ("/a/b.els.recover"));
    BOOST_REQUIRE (SessionService::recoveryFileFor (File()) == SessionService::untitledRecoveryFile());
    BOOST_REQUIRE (SessionService::recoveryFileFor (DataPath::defaultSessionDir()) == SessionService::untitledRecoveryFile());
    BOOST_REQUIRE (SessionService::untitledRecoveryFile().getParentDirectory() == DataPath::recoveryDir());
}

BOOST_AUTO_TEST_CASE (WriteRecoveryFileKeepsDirty)
{
    CleanState clean;
    auto& svc = sessions();
    dirtySession();

    BOOST_REQUIRE (svc.writeRecoveryFile());
    BOOST_REQUIRE (svc.hasRecoveryFile());
    BOOST_REQUIRE (svc.hasSessionChanged());

    auto xml = XmlDocument::parse (svc.currentRecoveryFile());
    BOOST_REQUIRE (xml != nullptr);
    const auto data = ValueTree::fromXml (*xml);
    BOOST_REQUIRE (data.hasType (types::Session));
    BOOST_REQUIRE_EQUAL ((int) data.getProperty (tags::version), EL_SESSION_VERSION);
}

BOOST_AUTO_TEST_CASE (AutosaveIfNeededHonoursInterval)
{
    CleanState clean;
    auto& svc = sessions();

    svc.autosaveIfNeeded();
    BOOST_REQUIRE (! svc.hasRecoveryFile());

    dirtySession();
    svc.setAutosaveInterval (RelativeTime::minutes (2));
    svc.autosaveIfNeeded();
    BOOST_REQUIRE (! svc.hasRecoveryFile());

    svc.setAutosaveInterval (RelativeTime (0.0));
    svc.autosaveIfNeeded();
    BOOST_REQUIRE (svc.hasRecoveryFile());
    svc.setAutosaveInterval (RelativeTime::minutes (2));
}

BOOST_AUTO_TEST_CASE (RecoverFromRestoresAndMarksDirty)
{
    CleanState clean;
    auto& svc = sessions();
    TempSession temp;

    svc.openFile (temp.file);
    pump();
    BOOST_REQUIRE (svc.getSessionFile() == temp.file);
    BOOST_REQUIRE (! svc.hasSessionChanged());

    renameActiveGraph ("Recovered");
    BOOST_REQUIRE (svc.hasSessionChanged());
    BOOST_REQUIRE (svc.writeRecoveryFile());
    const auto sidecar = SessionService::recoveryFileFor (temp.file);
    BOOST_REQUIRE (sidecar.existsAsFile());

    renameActiveGraph ("Changed Again");
    svc.resetChanges();
    BOOST_REQUIRE (! svc.hasSessionChanged());

    BOOST_REQUIRE (svc.recoverFrom (temp.file));
    BOOST_REQUIRE_EQUAL (et::context()->session()->getActiveGraph().getName().toStdString(), "Recovered");
    BOOST_REQUIRE (svc.getSessionFile() == temp.file);
    BOOST_REQUIRE (svc.hasSessionChanged());
    BOOST_REQUIRE (! sidecar.existsAsFile());

    pump (60);
    BOOST_REQUIRE (svc.hasSessionChanged());
}

BOOST_AUTO_TEST_CASE (SaveDeletesRecoveryFile)
{
    CleanState clean;
    auto& svc = sessions();
    TempSession temp;

    svc.openFile (temp.file);
    pump();
    renameActiveGraph ("Saved");
    BOOST_REQUIRE (svc.writeRecoveryFile());
    const auto sidecar = SessionService::recoveryFileFor (temp.file);
    BOOST_REQUIRE (sidecar.existsAsFile());

    svc.saveSession (false, false, false);
    BOOST_REQUIRE (! svc.hasSessionChanged());
    BOOST_REQUIRE (! sidecar.existsAsFile());
}

BOOST_AUTO_TEST_CASE (NewAndCloseDeleteRecoveryFile)
{
    CleanState clean;
    auto& svc = sessions();
    TempSession temp;

    svc.openFile (temp.file);
    pump();
    renameActiveGraph ("Pending");
    BOOST_REQUIRE (svc.writeRecoveryFile());
    const auto sidecar = SessionService::recoveryFileFor (temp.file);
    BOOST_REQUIRE (sidecar.existsAsFile());
    svc.resetChanges();
    svc.newSession();
    BOOST_REQUIRE (! sidecar.existsAsFile());

    dirtySession();
    BOOST_REQUIRE (svc.writeRecoveryFile());
    BOOST_REQUIRE (SessionService::untitledRecoveryFile().existsAsFile());
    svc.resetChanges();
    svc.closeSession();
    BOOST_REQUIRE (! SessionService::untitledRecoveryFile().existsAsFile());
}

BOOST_AUTO_TEST_CASE (AutosaveEnabledInStandaloneContext)
{
    BOOST_REQUIRE (sessions().isAutosaveEnabled());
}

BOOST_AUTO_TEST_SUITE_END()
