// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce/core.hpp>

namespace element {

/** Last-resort, process-wide crash logging for the standalone application.

    Installs a std::terminate handler and JUCE's application crash handler so
    that an uncaught exception or a fatal signal on any thread leaves a line
    (and a backtrace where possible) in the application log before the process
    dies. Nothing is ever saved from the crashing process.

    Standalone only: this must never be called from Context or a Service. A
    plugin lives inside a host process and must not replace the host's handlers.
*/
class CrashLog
{
public:
    /** Installs the handlers, appending to logFile.

        The path is captured now; the file is opened on demand with open(2),
        which is async-signal-safe, so log rotation never leaves a stale
        descriptor. Calling it again while installed does nothing.

        @param logFile The file to append crash records to.
    */
    static void install (const juce::File& logFile);

    /** Restores the previous terminate handler and resets the signals JUCE
        hooked back to their defaults. */
    static void uninstall();

    /** Returns true while the handlers are installed. */
    static bool isInstalled();

    /** Describes std::current_exception() for the log.

        @return "std::exception: <what>", "juce::String: <text>", "const char*: <text>",
                "unknown" for any other type, or "no active exception".
    */
    static juce::String describeCurrentException();
};

} // namespace element
