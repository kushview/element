// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/** Set to 1 (or define via CMake) to log timing of session load phases to the
    main log.  Kept off by default: probes compile to nothing.
 */
#ifndef EL_TRACE_SESSION_LOAD
#define EL_TRACE_SESSION_LOAD 0
#endif

#if EL_TRACE_SESSION_LOAD

#include <map>

#include <element/juce/core.hpp>

namespace element {

/** RAII timer which logs "[load] <label>: N.NN ms" when it goes out of scope.

    Output goes through juce::Logger::writeToLog, which routes to the
    application Log (log/main.log) when installed, so traces are visible in
    release builds.
*/
class ScopedLoadTrace
{
public:
    explicit ScopedLoadTrace (const juce::String& traceLabel)
        : label (traceLabel),
          startTicks (juce::Time::getHighResolutionTicks())
    {
    }

    ~ScopedLoadTrace()
    {
        const auto elapsed = juce::Time::getHighResolutionTicks() - startTicks;
        const auto ms = 1000.0 * juce::Time::highResolutionTicksToSeconds (elapsed);
        juce::Logger::writeToLog (juce::String ("[load] ") + label + ": " + juce::String (ms, 2) + " ms");
    }

    /** Increments and logs a named counter. Use for counting how often a hot
        function fires during a load (e.g. rendering sequence rebuilds). */
    static void count (const char* what)
    {
        static juce::CriticalSection lock;
        static std::map<juce::String, int> counters;
        juce::ScopedLock sl (lock);
        const auto total = ++counters[what];
        juce::Logger::writeToLog (juce::String ("[load] count ") + what + ": " + juce::String (total));
    }

private:
    const juce::String label;
    const juce::int64 startTicks;

    JUCE_DECLARE_NON_COPYABLE (ScopedLoadTrace)
};

} // namespace element

#define EL_LOAD_TRACE_PASTE2(a, b) a##b
#define EL_LOAD_TRACE_PASTE(a, b) EL_LOAD_TRACE_PASTE2 (a, b)
#define EL_LOAD_TRACE(label) ::element::ScopedLoadTrace EL_LOAD_TRACE_PASTE (elScopedLoadTrace_, __LINE__) (label)
#define EL_LOAD_TRACE_COUNT(what) ::element::ScopedLoadTrace::count (what)

#else

#define EL_LOAD_TRACE(label)
#define EL_LOAD_TRACE_COUNT(what)

#endif
