// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include <exception>
#include <stdexcept>

#include "crashlog.hpp"

using namespace element;
using namespace juce;

BOOST_AUTO_TEST_SUITE (CrashLogTests)

BOOST_AUTO_TEST_CASE (DescribeStdException)
{
    try {
        throw std::runtime_error ("boom");
    } catch (...) {
        const auto text = CrashLog::describeCurrentException();
        BOOST_REQUIRE_MESSAGE (text.contains ("boom"), text.toStdString());
        BOOST_REQUIRE (text.startsWith ("std::exception"));
    }
}

BOOST_AUTO_TEST_CASE (DescribeJuceString)
{
    try {
        throw String ("stringy");
    } catch (...) {
        BOOST_REQUIRE (CrashLog::describeCurrentException().contains ("stringy"));
    }
}

BOOST_AUTO_TEST_CASE (DescribeUnknown)
{
    try {
        throw 42;
    } catch (...) {
        BOOST_REQUIRE_EQUAL (CrashLog::describeCurrentException().toStdString(), "unknown");
    }
}

BOOST_AUTO_TEST_CASE (DescribeNothing)
{
    BOOST_REQUIRE_EQUAL (CrashLog::describeCurrentException().toStdString(), "no active exception");
}

BOOST_AUTO_TEST_CASE (InstallReplacesAndUninstallRestoresTerminate)
{
    const auto logFile = File::createTempFile ("crashlog");
    const auto previous = std::get_terminate();

    BOOST_REQUIRE (! CrashLog::isInstalled());
    CrashLog::install (logFile);
    BOOST_REQUIRE (CrashLog::isInstalled());
    const auto installedHandler = std::get_terminate();
    BOOST_REQUIRE (installedHandler != previous);

    CrashLog::install (logFile);
    BOOST_REQUIRE (std::get_terminate() == installedHandler);

    CrashLog::uninstall();
    BOOST_REQUIRE (! CrashLog::isInstalled());
    BOOST_REQUIRE (std::get_terminate() == previous);
    logFile.deleteFile();
}

BOOST_AUTO_TEST_SUITE_END()
