#include "Tests.h"

namespace Element {

class LinearFadeTest : public UnitTestBase
{
public:
    LinearFadeTest() : UnitTestBase ("LinearFade", "engine", "linearFade") { }
    virtual ~LinearFadeTest() { }

    void runTest() override
    {
        testLinearFade();
    }

private:
    void testLinearFade()
    {
        beginTest ("linear fade");
        LinearFade fader;
        fader.setSampleRate (44100.0);
        fader.setLength (1.0);
        fader.setFadesIn (false);

        expect (! fader.isActive());
        fader.startFading();
        expect (fader.isActive());

        int frame = 0;

        while (fader.isActive())
        {
            float gain = fader.getNextEnvelopeValue();
            frame++;
        }
        
        DBG("frames processed: " << frame);
        expect (fader.isActive() == false);
        expect (fader.getCurrentEnvelopeValue() == 1.0);
    }
};

static LinearFadeTest sLinearFadeTest;

}
