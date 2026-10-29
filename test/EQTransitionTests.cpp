// SPDX-FileCopyrightText: 2026 Kushview, LLC
// Author: jiri-god <318975419+jiri-god@users.noreply.github.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include <boost/test/unit_test.hpp>
#include "nodes/eqfilter.hpp"
using namespace element;
using namespace juce;
BOOST_AUTO_TEST_SUITE (EQTransitionTests)
BOOST_AUTO_TEST_CASE (EQShapeTransitionsRemainContinuousAtDifferentRates)
{
    for (double rate : { 44100., 48000., 192000. }) {
        EQFilter filter;
        filter.reset (rate);
        filter.setFrequency (1000);
        filter.setQ (.707f);
        filter.setGain (1);
        for (int i = 0; i < int (rate); ++i) {
            float x = 1;
            filter.processBlock (&x, 1);
        }
        float before = 1;
        filter.processBlock (&before, 1);
        filter.setShape (EQFilter::HighPass);
        float after = 1;
        filter.processBlock (&after, 1);
        BOOST_CHECK_SMALL (after - before, 1e-4f);
        for (int i = 0; i < int (rate * .1); ++i) {
            if (i % 100 == 0)
                filter.setShape ((i / 100) % 2 ? EQFilter::LowPass : EQFilter::HighPass);
            float x = .25f;
            filter.processBlock (&x, 1);
            BOOST_CHECK (std::isfinite (x));
            BOOST_CHECK_LT (std::abs (x), 4.f);
        }
    }
}
BOOST_AUTO_TEST_CASE (PrepareClampsCutoffUsingTheNewRate)
{
    EQFilterProcessor processor;
    auto* frequency = dynamic_cast<AudioParameterFloat*> (processor.getParameters()[0]);
    auto* gain = dynamic_cast<AudioParameterFloat*> (processor.getParameters()[2]);
    BOOST_REQUIRE (frequency != nullptr);
    BOOST_REQUIRE (gain != nullptr);
    *gain = 6.0f;
    processor.prepareToPlay (24000, 128);
    *frequency = 20000.0f;
    processor.releaseResources();
    processor.prepareToPlay (48000, 128);
    BOOST_CHECK_CLOSE (processor.getMagnitudeAtFreq (20000), Decibels::decibelsToGain (6.0f), 0.01f);
    processor.releaseResources();
}
BOOST_AUTO_TEST_SUITE_END()
