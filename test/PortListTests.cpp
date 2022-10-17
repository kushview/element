/*
    This file is part of Element
    Copyright (C) 2018-2019  Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <boost/test/unit_test.hpp>
#include "JuceHeader.h"
#include "porttype.hpp"

using PortType = element::PortType;

BOOST_AUTO_TEST_SUITE (PortListTests)

BOOST_AUTO_TEST_CASE (Types)
{
    element::PortList ports;
    int index = 0;
    for (int control = 0; control < 16; ++control) {
        String symbol = "control_";
        symbol << control;
        ports.add (PortType::Control, index++, control, symbol, "Control", true);
    }

    ports.add (PortType::Midi, index++, 0, "midi_in", "MIDI", true);
    ports.add (PortType::Midi, index++, 0, "midi_out", "MIDI", false);

    for (int port = 0; port < 16; ++port) {
        BOOST_REQUIRE (ports.getType (port) == PortType::Control);
        BOOST_REQUIRE (ports.isInput (port));
        BOOST_REQUIRE (ports.getChannelForPort (port) == port);
    }

    const PortType midiType (PortType::Midi);
    BOOST_REQUIRE (ports.isInput (16) == true);
    BOOST_REQUIRE (ports.getType (16) == midiType);
    BOOST_REQUIRE (ports.isOutput (17) == true);
    BOOST_REQUIRE (ports.getType (17) == midiType);
}

BOOST_AUTO_TEST_SUITE_END()
