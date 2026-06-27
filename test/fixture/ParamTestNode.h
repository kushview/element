// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include "fixture/TestNode.h"

namespace element {

/** A TestNode that exposes a configurable number of real (Control) input
    parameters, so MIDI-mapping targets can be exercised in unit tests.
*/
class ParamTestNode : public TestNode {
public:
    /** @param numParams Number of control input parameters to expose. */
    explicit ParamTestNode (int numParams = 2)
        : TestNode (2, 2, 1, 1),
          numParameters (numParams)
    {
        ParamTestNode::refreshPorts();
    }

    void refreshPorts() override
    {
        PortList newPorts;
        uint32 port = 0;

        for (int c = 0; c < numAudioIns; ++c)
            newPorts.add (PortType::Audio, port++, c, String ("audio_in_") + String (c + 1), String ("In ") + String (c + 1), true);
        for (int c = 0; c < numMidiIns; ++c)
            newPorts.add (PortType::Midi, port++, c, String ("midi_in_") + String (c + 1), String ("MIDI In ") + String (c + 1), true);

        for (int c = 0; c < numParameters; ++c)
            newPorts.addControl ((int) port++, c, String ("param_") + String (c + 1), String ("Param ") + String (c + 1), 0.0f, 1.0f, 0.0f, true);

        for (int c = 0; c < numAudioOuts; ++c)
            newPorts.add (PortType::Audio, port++, c, String ("audio_out_") + String (c + 1), String ("Out ") + String (c + 1), false);
        for (int c = 0; c < numMidiOuts; ++c)
            newPorts.add (PortType::Midi, port++, c, String ("midi_out_") + String (c + 1), String ("MIDI Out ") + String (c + 1), false);

        setPorts (newPorts);
    }

private:
    int numParameters = 2;
};

} // namespace element
