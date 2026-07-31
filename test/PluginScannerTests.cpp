// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include <element/juce/core.hpp>
#include <element/plugins.hpp>
#include <element/settings.hpp>

using namespace element;

namespace {

struct RecordingListener : public PluginScanner::Listener
{
    std::atomic<int> finishedCount { 0 };
    void audioPluginScanFinished() override { ++finishedCount; }
};

/** Returns the element executable to use for integration tests that spawn a
    real scanner worker, or an invalid File when they should be skipped.

    Opt-in: set EL_TEST_SCANNER_INTEGRATION=1 to use the executable from this
    build tree, or point EL_TEST_SCANNER_EXE at a specific binary, e.g.
      EL_TEST_SCANNER_INTEGRATION=1 ctest --test-dir build -R PluginScannerTests
*/
static juce::File integrationScannerExe()
{
    const auto exe = juce::SystemStats::getEnvironmentVariable ("EL_TEST_SCANNER_EXE", {});
    if (exe.isNotEmpty())
        return juce::File (exe);

#ifdef EL_TEST_SCANNER_EXE_PATH
    if (juce::SystemStats::getEnvironmentVariable ("EL_TEST_SCANNER_INTEGRATION", "0") == "1")
        return juce::File (EL_TEST_SCANNER_EXE_PATH);
#endif

    return {};
}

/** Creates a temp directory holding fake (non-loadable) .vst3 files. */
static juce::File makeGarbageDir (std::initializer_list<const char*> names)
{
    auto dir = juce::File::createTempFile ("elscan");
    dir.deleteFile();
    BOOST_REQUIRE (dir.createDirectory());
    for (const auto* name : names)
        BOOST_REQUIRE (dir.getChildFile (name).replaceWithText ("not a plugin"));
    return dir;
}

/** Builds a PropertiesFile whose VST3 search path is set to searchPath. */
static std::unique_ptr<juce::PropertiesFile> makeProps (const juce::File& dir, const juce::String& searchPath)
{
    juce::PropertiesFile::Options opts;
    opts.storageFormat = juce::PropertiesFile::storeAsXML;
    auto props = std::make_unique<juce::PropertiesFile> (dir.getChildFile ("test.settings"), opts);
    props->setValue (juce::String (Settings::lastPluginScanPathPrefix) + "VST3", searchPath);
    return props;
}

} // namespace

BOOST_AUTO_TEST_SUITE (PluginScannerTests)

/** A missing scanner executable must still finish the scan and must not
    blacklist anything. */
BOOST_AUTO_TEST_CASE (MissingScannerExeFinishesAndDoesNotBlacklist)
{
    PluginManager manager;
    manager.addDefaultFormats();

    std::unique_ptr<PluginScanner> scanner (manager.createAudioPluginScanner());
    RecordingListener listener;
    scanner->addListener (&listener);
    scanner->setScannerExe (juce::File ("/path/does/not/exist/element-scanner"));

    scanner->scanForAudioPlugins (juce::StringArray { "VST3" });
    BOOST_REQUIRE (scanner->waitForScanToFinish (10000));

    BOOST_CHECK_EQUAL (listener.finishedCount.load(), 1);
    BOOST_CHECK (! scanner->isScanning());
    BOOST_CHECK (manager.getKnownPlugins().getBlacklistedFiles().isEmpty());
    BOOST_CHECK (scanner->getFailedFiles().isEmpty());
    BOOST_CHECK (scanner->getLastScanError().isNotEmpty());

    scanner->removeListener (&listener);
}

/** A scanner executable that launches but never connects is an
    infrastructure failure: no plugin may be blacklisted, and the circuit
    breaker must abort the scan with an error. This is the regression test
    for one failed plugin poisoning the remainder of a scan. */
BOOST_AUTO_TEST_CASE (BrokenScannerExeNeverBlacklists)
{
    // A real executable that exits immediately without ever connecting.
#if JUCE_WINDOWS
    const juce::File brokenExe ("C:\\Windows\\System32\\where.exe");
#else
    const juce::File brokenExe ("/bin/true");
#endif
    if (! brokenExe.existsAsFile())
        return;

    auto tmp = makeGarbageDir ({ "a.vst3", "b.vst3", "c.vst3" });
    auto props = makeProps (tmp, tmp.getFullPathName());

    PluginManager manager;
    manager.addDefaultFormats();
    manager.setPropertiesFile (props.get());

    if (! manager.isAudioPluginFormatSupported ("VST3"))
        return;

    std::unique_ptr<PluginScanner> scanner (manager.createAudioPluginScanner());
    RecordingListener listener;
    scanner->addListener (&listener);
    scanner->setScannerExe (brokenExe);
    scanner->setLaunchTimeout (1000);
    scanner->setPerPluginTimeout (2000);
    scanner->setMaxConsecutiveFailures (2);

    scanner->scanForAudioPlugins (juce::StringArray { "VST3" });
    BOOST_REQUIRE (scanner->waitForScanToFinish (30000));

    BOOST_CHECK_EQUAL (listener.finishedCount.load(), 1);
    BOOST_CHECK (manager.getKnownPlugins().getBlacklistedFiles().isEmpty());
    BOOST_CHECK (scanner->getFailedFiles().isEmpty());
    BOOST_CHECK (scanner->getLastScanError().isNotEmpty());

    scanner->removeListener (&listener);
    scanner.reset();
    manager.setPropertiesFile (nullptr);
    tmp.deleteRecursively();
}

/** Full round trip against the real element executable: worker launches,
    completes the ready handshake, scans garbage files without hanging, and
    reports them failed without touching innocent state. */
BOOST_AUTO_TEST_CASE (RealWorkerScansGarbageWithoutHanging)
{
    const auto exe = integrationScannerExe();
    if (! exe.existsAsFile())
        return;

    auto tmp = makeGarbageDir ({ "fake1.vst3", "fake2.vst3" });
    auto props = makeProps (tmp, tmp.getFullPathName());

    PluginManager manager;
    manager.addDefaultFormats();
    manager.setPropertiesFile (props.get());

    if (! manager.isAudioPluginFormatSupported ("VST3"))
        return;

    std::unique_ptr<PluginScanner> scanner (manager.createAudioPluginScanner());
    RecordingListener listener;
    scanner->addListener (&listener);
    scanner->setScannerExe (exe);
    scanner->setPerPluginTimeout (30000);

    scanner->scanForAudioPlugins (juce::StringArray { "VST3" });
    BOOST_REQUIRE (scanner->waitForScanToFinish (60000));

    BOOST_CHECK_EQUAL (listener.finishedCount.load(), 1);
    BOOST_CHECK (scanner->getLastScanError().isEmpty());
    // Garbage files legitimately fail to load: reported failed and blacklisted.
    BOOST_CHECK_EQUAL (scanner->getFailedFiles().size(), 2);
    BOOST_CHECK_EQUAL (manager.getKnownPlugins().getBlacklistedFiles().size(), 2);

    scanner->removeListener (&listener);
    scanner.reset();
    manager.setPropertiesFile (nullptr);
    tmp.deleteRecursively();
}

#if defined(EL_TEST_BADPLUGINS_DIR)
/** THE regression test for the reported bug: a plugin that crashes the
    worker process must be blacklisted alone, the worker must be relaunched,
    and every plugin after it must still be scanned on its own merits.

    The crasher (built by test/CMakeLists.txt from fixture/badplugin.c) is
    placed first in the search path via a two-directory FileSearchPath so the
    crash happens before the other files are visited. */
BOOST_AUTO_TEST_CASE (CrashingPluginDoesNotPoisonScan)
{
    const auto exe = integrationScannerExe();
    const juce::File crashDir = juce::File (EL_TEST_BADPLUGINS_DIR).getChildFile ("crasher");
    if (! exe.existsAsFile() || ! crashDir.isDirectory())
        return;

    auto tmp = makeGarbageDir ({ "after1.vst3", "after2.vst3" });
    auto props = makeProps (tmp, crashDir.getFullPathName() + ";" + tmp.getFullPathName());

    PluginManager manager;
    manager.addDefaultFormats();
    manager.setPropertiesFile (props.get());

    std::unique_ptr<PluginScanner> scanner (manager.createAudioPluginScanner());
    RecordingListener listener;
    scanner->addListener (&listener);
    scanner->setScannerExe (exe);
    scanner->setPerPluginTimeout (30000);

    scanner->scanForAudioPlugins (juce::StringArray { "VST3" });
    BOOST_REQUIRE (scanner->waitForScanToFinish (120000));

    BOOST_CHECK_EQUAL (listener.finishedCount.load(), 1);
    BOOST_CHECK (scanner->getLastScanError().isEmpty());

    // The crasher is blacklisted for crashing; the files after it must have
    // been scanned by a relaunched worker and judged on their own merits.
    // If the crash had poisoned the scan they would be scanner-unavailable
    // failures instead, tripping the circuit breaker.
    const auto& blacklist = manager.getKnownPlugins().getBlacklistedFiles();
    BOOST_CHECK_EQUAL (scanner->getFailedFiles().size(), 3);
    BOOST_CHECK_EQUAL (blacklist.size(), 3);
    BOOST_CHECK (blacklist.contains (crashDir.getChildFile ("crasher.vst3").getFullPathName()));

    scanner->removeListener (&listener);
    scanner.reset();
    manager.setPropertiesFile (nullptr);
    tmp.deleteRecursively();
}

/** A plugin that hangs the worker must be killed at the per-plugin timeout,
    treated as crashed, and the scan must continue with a fresh worker. */
BOOST_AUTO_TEST_CASE (HangingPluginTimesOutAndScanContinues)
{
    const auto exe = integrationScannerExe();
    const juce::File hangDir = juce::File (EL_TEST_BADPLUGINS_DIR).getChildFile ("hanger");
    if (! exe.existsAsFile() || ! hangDir.isDirectory())
        return;

    auto tmp = makeGarbageDir ({ "after1.vst3", "after2.vst3" });
    auto props = makeProps (tmp, hangDir.getFullPathName() + ";" + tmp.getFullPathName());

    PluginManager manager;
    manager.addDefaultFormats();
    manager.setPropertiesFile (props.get());

    std::unique_ptr<PluginScanner> scanner (manager.createAudioPluginScanner());
    RecordingListener listener;
    scanner->addListener (&listener);
    scanner->setScannerExe (exe);
    scanner->setPerPluginTimeout (3000);

    scanner->scanForAudioPlugins (juce::StringArray { "VST3" });
    BOOST_REQUIRE (scanner->waitForScanToFinish (120000));

    BOOST_CHECK_EQUAL (listener.finishedCount.load(), 1);
    BOOST_CHECK (scanner->getLastScanError().isEmpty());

    const auto& blacklist = manager.getKnownPlugins().getBlacklistedFiles();
    BOOST_CHECK_EQUAL (scanner->getFailedFiles().size(), 3);
    BOOST_CHECK_EQUAL (blacklist.size(), 3);
    BOOST_CHECK (blacklist.contains (hangDir.getChildFile ("hanger.vst3").getFullPathName()));

    scanner->removeListener (&listener);
    scanner.reset();
    manager.setPropertiesFile (nullptr);
    tmp.deleteRecursively();
}
#endif

BOOST_AUTO_TEST_SUITE_END()
