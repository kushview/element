#include <boost/test/unit_test.hpp>
#include "engine/linearfade.hpp"

using namespace element;
using namespace juce;

class LinearFadeUnit {
public:
    void runTest()
    {
        testLinearFade();
    }

private:
    void testLinearFade()
    {
        LinearFade fader;
        fader.setSampleRate (44100.0);
        fader.setLength (0.02);
        fader.setFadesIn (false);

        BOOST_REQUIRE (! fader.isActive());
        fader.startFading();
        BOOST_REQUIRE (fader.isActive());

        int frame = 0;
        juce::ignoreUnused (frame);

        while (fader.isActive()) {
            float gain = fader.getNextEnvelopeValue();
            frame++;
        }

        BOOST_REQUIRE (fader.isActive() == false);
        BOOST_REQUIRE (fader.getCurrentEnvelopeValue() == 0.0);
        fader.reset();
        BOOST_REQUIRE (fader.getCurrentEnvelopeValue() == 1.0);
    }
};

BOOST_AUTO_TEST_SUITE (LinearFadeTest)

BOOST_AUTO_TEST_CASE (Basics)
{
    LinearFadeUnit().runTest();
}

BOOST_AUTO_TEST_SUITE_END()
