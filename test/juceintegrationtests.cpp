// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <boost/test/unit_test.hpp>

// Test that all JUCE integration headers compile
#include <element/juce.hpp>
#include <element/juce/audio_basics.hpp>
#include <element/juce/audio_devices.hpp>
#include <element/juce/audio_formats.hpp>
#include <element/juce/audio_processors.hpp>
#include <element/juce/core.hpp>
#include <element/juce/data_structures.hpp>
#include <element/juce/dsp.hpp>
#include <element/juce/events.hpp>
#include <element/juce/graphics.hpp>
#include <element/juce/gui_basics.hpp>
#include <element/juce/gui_extra.hpp>
#include <element/juce/osc.hpp>

BOOST_AUTO_TEST_SUITE (JuceIntegrationTests)

BOOST_AUTO_TEST_CASE (juce_headers_compile)
{
    // This test simply verifies that all JUCE integration headers
    // compile successfully. No runtime functionality to test.
    BOOST_REQUIRE (true);
}

BOOST_AUTO_TEST_CASE (juce_types_available)
{
    // Verify that common JUCE types are available through our headers
    juce::String str ("test");
    BOOST_REQUIRE (str.isNotEmpty());
    
    juce::MidiMessage msg = juce::MidiMessage::noteOn (1, 60, 0.8f);
    BOOST_REQUIRE (msg.isNoteOn());
    
    juce::AudioBuffer<float> buffer (2, 512);
    BOOST_REQUIRE_EQUAL (buffer.getNumChannels(), 2);
    BOOST_REQUIRE_EQUAL (buffer.getNumSamples(), 512);
}

BOOST_AUTO_TEST_SUITE_END()
