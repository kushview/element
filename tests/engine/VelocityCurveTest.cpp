#include "Tests.h"

namespace Element {

class VelocityCurveTest : public UnitTestBase
{
public:
    VelocityCurveTest() : UnitTestBase ("Velocity Curve", "dsp") { }

    virtual void initialise() override { }
    virtual void shutdown() override { }
    virtual void runTest() override
    {
        VelocityCurve curve;
        beginTest (curve.getModeName());
        expect (curve.getMode() == VelocityCurve::Linear);
        expect (0.6f == curve.process (0.6f));

        curve.setMode (VelocityCurve::Soft_1);
        beginTest (curve.getModeName());
        expect (0.f == std::floor (127.f * curve.process (0.f)));
        expect (90.0  == std::floor (127.f * curve.process (100.f / 127.f)));
        expect (38.0  == std::floor (127.f * curve.process (50.f / 127.f)));
        expect (17.0  == std::floor (127.f * curve.process (25.f / 127.f)));
        expect (127.f == std::floor (127.f * curve.process (1.f)));

        curve.setMode (VelocityCurve::Hard_1);
        beginTest (curve.getModeName());
        expect (0.f == std::floor (127.f * curve.process (0.f)));
        expect (107.0  == std::floor (127.f * curve.process (100.f / 127.f)));
        expect (62.0  == std::floor (127.f * curve.process (50.f / 127.f)));
        expect (33.0  == std::floor (127.f * curve.process (25.f / 127.f)));
        expect (127.f == std::floor (127.f * curve.process (1.f)));
    }
};

static VelocityCurveTest sVelocityCurveTest;

}
