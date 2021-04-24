/*
    This file is part of Element
    Copyright (C) 2018-2021  Kushview, LLC.  All rights reserved.

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

#include "Tests.h"
#include "plugins/PluginProcessor.h"

namespace Element {

class PluginTests : public UnitTestBase
{
public:
    PluginTests()
        : UnitTestBase ("Plugins", "Plugins", "plugins") {}
    ~PluginTests() override {}

    void runTest() override
    {
        testDefaultSetup();
        testElementInstrument();
        testElementEffect();
        testElementMidiEffect();
    }

private:
    void testDefaultSetup()
    {
        beginTest ("default setup");
        PluginProcessor plugin;
        expect (plugin.getVariant() == PluginProcessor::Instrument);
        expect (plugin.getTotalNumInputChannels() == 0);
        expect (plugin.getTotalNumOutputChannels() == 2);
        expect (plugin.enableAllBuses());
        expect (plugin.getTotalNumInputChannels() == 2);
        expect (plugin.getTotalNumOutputChannels() == 2);

        expect (plugin.isMidiEffect() == false);
    }

    void testElementInstrument()
    {
        beginTest ("instrument");
        PluginProcessor plugin (PluginProcessor::Instrument, 16);
        expect (plugin.getVariant() == PluginProcessor::Instrument);
        expect (plugin.getTotalNumInputChannels() == 0);
        expect (plugin.getTotalNumOutputChannels() == 2);
        expect (plugin.enableAllBuses());
        expect (plugin.getTotalNumInputChannels() == 34);
        expect (plugin.getTotalNumOutputChannels() == 34);

        expect (plugin.isMidiEffect() == false);

        auto layout = plugin.getBusesLayout();
        for (int i = 0; i < layout.inputBuses.size(); ++i)
            layout.inputBuses.getReference(i) = AudioChannelSet::disabled();
        layout.outputBuses.getReference(0) = AudioChannelSet::create7point1();
        expect (plugin.checkBusesLayoutSupported (layout));
        expect (plugin.setBusesLayout (layout));
        expect (plugin.getTotalNumInputChannels() == 0);
        expect (plugin.getTotalNumOutputChannels() == 8 + (2 * 16));

        for (int i = 0; i < layout.outputBuses.size(); ++i)
            layout.outputBuses.getReference(i) = AudioChannelSet::disabled();
        expect (! plugin.setBusesLayout (layout));
    }

    void testElementEffect()
    {
        beginTest ("effect");
        PluginProcessor plugin (PluginProcessor::Effect, 16);
        expect (plugin.getVariant() == PluginProcessor::Effect);
        expect (plugin.getTotalNumInputChannels() == 2);
        expect (plugin.getTotalNumOutputChannels() == 2);
        expect (plugin.enableAllBuses());
        expect (plugin.getTotalNumInputChannels() == 34);
        expect (plugin.getTotalNumOutputChannels() == 34);

        expect (plugin.isMidiEffect() == false);
    }

    void testElementMidiEffect()
    {
        beginTest ("midi effect");
        PluginProcessor plugin (PluginProcessor::MidiEffect, 0);
        expect (plugin.getVariant() == PluginProcessor::MidiEffect);
        expect (plugin.getTotalNumInputChannels() == 0);
        expect (plugin.getTotalNumOutputChannels() == 0);
        expect (plugin.enableAllBuses());
        expect (plugin.getTotalNumInputChannels() == 0);
        expect (plugin.getTotalNumOutputChannels() == 0);

        expect (plugin.isMidiEffect() == true);
        AudioProcessor::BusesLayout layout;
        layout.inputBuses.add (AudioChannelSet::stereo());
        layout.outputBuses.add (AudioChannelSet::stereo());
        expect (! plugin.checkBusesLayoutSupported (layout));
    }
};

static PluginTests sPluginTests;
}
