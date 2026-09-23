// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce/core.hpp>
#include <element/juce/data_structures.hpp>
#include <element/juce/events.hpp>

namespace element {

/** Records which session a launch is about to open so the next launch can
    tell that the previous one died while opening it.

    The marker is written and flushed to disk before the session loads, and
    cleared only after the application has run for a short while afterwards,
    because a crash caused by session content can land on the audio thread
    after the load call has returned. Message thread only.
*/
class StartupGuard : private juce::Timer
{
public:
    /** Binds to the settings file. Also records that this run has not (yet)
        shut down cleanly, after noting whether the previous run did.

        @param props The user settings file to persist the marker in.
    */
    explicit StartupGuard (juce::PropertiesFile& props);

    /** Stops the confirmation timer. Does not clear the marker. */
    ~StartupGuard() override;

    /** Returns the session a previous launch recorded and never confirmed,
        or an invalid File when there is none. */
    juce::File pendingSession() const;

    /** Records the session about to be opened and flushes settings to disk.

        @param session The session file that is about to load.
    */
    void beginOpening (const juce::File& session);

    /** Clears the marker after the given delay, once. */
    void scheduleConfirm (int milliseconds = 10 * 1000);

    /** Clears the marker and flushes settings to disk. */
    void confirmClean();

    /** Returns true if the previous run never recorded a clean shutdown. */
    bool previousRunWasUnclean() const noexcept { return unclean; }

    /** Records a clean shutdown for this run. */
    void markCleanShutdown();

private:
    juce::PropertiesFile& props;
    bool unclean = false;

    void timerCallback() override;
};

} // namespace element
