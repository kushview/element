// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include <element/settings.hpp>

#include "startupguard.hpp"

using namespace element;
using namespace juce;

namespace {

struct TempProps {
    TempProps()
        : file (File::createTempFile ("conf"))
    {
        file.deleteFile();
    }

    ~TempProps() { file.deleteFile(); }

    std::unique_ptr<PropertiesFile> open()
    {
        PropertiesFile::Options options;
        options.storageFormat = PropertiesFile::storeAsXML;
        return std::make_unique<PropertiesFile> (file, options);
    }

    File file;
};

const File sessionFile ("/tmp/element-startupguard-test/session.els");

} // namespace

BOOST_AUTO_TEST_SUITE (StartupGuardTests)

BOOST_AUTO_TEST_CASE (PendingIsEmptyByDefault)
{
    TempProps temp;
    auto props = temp.open();
    StartupGuard guard (*props);
    BOOST_REQUIRE (guard.pendingSession() == File());
    BOOST_REQUIRE (! guard.previousRunWasUnclean());
}

BOOST_AUTO_TEST_CASE (BeginOpeningPersistsAcrossReopen)
{
    TempProps temp;
    {
        auto props = temp.open();
        StartupGuard guard (*props);
        guard.beginOpening (sessionFile);
    }
    auto props = temp.open();
    StartupGuard guard (*props);
    BOOST_REQUIRE (guard.pendingSession() == sessionFile);
}

BOOST_AUTO_TEST_CASE (ConfirmCleanClears)
{
    TempProps temp;
    {
        auto props = temp.open();
        StartupGuard guard (*props);
        guard.beginOpening (sessionFile);
        guard.confirmClean();
    }
    auto props = temp.open();
    StartupGuard guard (*props);
    BOOST_REQUIRE (guard.pendingSession() == File());
}

BOOST_AUTO_TEST_CASE (ScheduledConfirmClears)
{
    TempProps temp;
    auto props = temp.open();
    StartupGuard guard (*props);
    guard.beginOpening (sessionFile);
    guard.scheduleConfirm (20);
    BOOST_REQUIRE (guard.pendingSession() == sessionFile);
    MessageManager::getInstance()->runDispatchLoopUntil (150);
    BOOST_REQUIRE (guard.pendingSession() == File());
}

BOOST_AUTO_TEST_CASE (DestructionDoesNotClear)
{
    TempProps temp;
    {
        auto props = temp.open();
        StartupGuard guard (*props);
        guard.beginOpening (sessionFile);
        guard.scheduleConfirm (10 * 1000);
    }
    auto props = temp.open();
    StartupGuard guard (*props);
    BOOST_REQUIRE (guard.pendingSession() == sessionFile);
}

BOOST_AUTO_TEST_CASE (UncleanRunIsDetectedUntilMarkedClean)
{
    TempProps temp;
    {
        auto props = temp.open();
        StartupGuard first (*props); // records "not clean" for this run
        BOOST_REQUIRE (! first.previousRunWasUnclean());
    }
    {
        auto props = temp.open();
        StartupGuard second (*props); // previous run never marked clean
        BOOST_REQUIRE (second.previousRunWasUnclean());
        second.markCleanShutdown();
        props->save();
    }
    auto props = temp.open();
    StartupGuard third (*props);
    BOOST_REQUIRE (! third.previousRunWasUnclean());
}

BOOST_AUTO_TEST_SUITE_END()
