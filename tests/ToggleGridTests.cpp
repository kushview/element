#include "Tests.h"
#include "engine/LinearFade.h"

namespace Element {

class ToggleGridTest : public UnitTestBase
{
public:
    ToggleGridTest() : UnitTestBase ("Toggle Grid", "engine", "toggleGrid") { }
    virtual ~ToggleGridTest() { }

    void runTest() override
    {
        testToggleGrid();
        testLinearFade();
    }

private:
    void testToggleGrid()
    {
        ToggleGrid grid1 (4, 4);
        ToggleGrid grid2 (4, 4);
        ToggleGrid grid3 (3, 4);

        beginTest ("sameSizeAs");
        expect (grid1.sameSizeAs (grid2));
        expect (! grid1.sameSizeAs (grid3));

        beginTest ("initial values");
        for (int i = 0; i < grid1.getNumInputs(); ++i)
            for (int o = 0; o < grid1.getNumOutputs(); ++o)
                expect (grid1.get (i, 0) == false);
        
        beginTest ("get/set");
        grid1.set (2, 2, true);
        expect (grid1.get (2, 2) == true);
        expect (grid2.get (2, 2) == false);

        beginTest ("swapWith");
        grid1.swapWith (grid2);
        expect (grid1.get (2, 2) == false);
        expect (grid2.get (2, 2) == true);
    }

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

static ToggleGridTest sToggleGridTest;

}
