// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/settings.hpp>

#include "startupguard.hpp"

namespace element {

StartupGuard::StartupGuard (juce::PropertiesFile& p)
    : props (p)
{
    unclean = props.containsKey (Settings::cleanShutdownKey)
              && ! props.getBoolValue (Settings::cleanShutdownKey);
    props.setValue (Settings::cleanShutdownKey, false);
    props.save();
}

StartupGuard::~StartupGuard()
{
    stopTimer();
}

juce::File StartupGuard::pendingSession() const
{
    const auto path = props.getValue (Settings::startupPendingSessionKey);
    return juce::File::isAbsolutePath (path) ? juce::File (path) : juce::File();
}

void StartupGuard::beginOpening (const juce::File& session)
{
    props.setValue (Settings::startupPendingSessionKey, session.getFullPathName());
    props.save();
}

void StartupGuard::scheduleConfirm (int milliseconds)
{
    startTimer (juce::jmax (1, milliseconds));
}

void StartupGuard::confirmClean()
{
    stopTimer();
    if (! props.containsKey (Settings::startupPendingSessionKey))
        return;
    props.removeValue (Settings::startupPendingSessionKey);
    props.save();
}

void StartupGuard::markCleanShutdown()
{
    props.setValue (Settings::cleanShutdownKey, true);
}

void StartupGuard::timerCallback()
{
    confirmClean();
}

} // namespace element
