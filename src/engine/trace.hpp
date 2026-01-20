// SPDX-FileCopyrightText: 2023 Kushview, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce/audio_basics.hpp>

namespace element {

inline static void traceMidi (const juce::MidiMessage& msg, const int frame = -1)
{
    if (msg.isMidiClock())
    {
        DBG ("clock:");
    }
    if (msg.isNoteOn())
    {
        std::clog << "NOTE ON: " << msg.getMidiNoteName (msg.getNoteNumber(), true, false, 4).toStdString() << std::endl;
    }
    if (msg.isNoteOff())
    {
        std::clog << "NOTE OFF: " << msg.getMidiNoteName (msg.getNoteNumber(), true, false, 4).toStdString() << std::endl;
    }

    if (msg.isController())
    {
        msg.isControllerOfType (0);
        DBG ("MIDI CC: " << msg.getControllerNumber() << ":" << msg.getControllerValue());
    }

    if (msg.isProgramChange())
    {
        DBG ("program change: " << msg.getProgramChangeNumber() << " ch: " << msg.getChannel());
    }

    if (frame >= 0 && (msg.isAllNotesOff() || msg.isAllSoundOff()))
    {
        DBG ("got it: " << frame);
    }
}

inline static void traceMidi (juce::MidiBuffer& buf)
{
    for (const auto i : buf)
        traceMidi (i.getMessage(), i.samplePosition);
}
} // namespace element
