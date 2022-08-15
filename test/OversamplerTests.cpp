#include <boost/test/unit_test.hpp>
#include "engine/oversampler.hpp"

using namespace element;

BOOST_AUTO_TEST_SUITE (OversamplerTests)

BOOST_AUTO_TEST_CASE (Basics)
{
    Oversampler<float> os;
    BOOST_REQUIRE (os.getNumProcessors() == 0);
    BOOST_REQUIRE (os.getProcessor (0) == nullptr);
    BOOST_REQUIRE (os.getLatencySamples(0) == 0);
    BOOST_REQUIRE (os.getFactor (0) == 1);
    os.prepare (2, 1024);
    BOOST_REQUIRE (os.getNumProcessors() == 3);
    BOOST_REQUIRE_EQUAL (os.getFactor(0), 2);
    for (int i = 0; i < os.getNumProcessors(); ++i)
    {
        BOOST_REQUIRE (nullptr != os.getProcessor (i));
        if (auto* const proc = os.getProcessor (i))
        {
            size_t factora = static_cast<size_t> (std::pow (2.0, (double)(i + 1)));
            size_t factorb = proc->getOversamplingFactor();
            BOOST_REQUIRE_EQUAL (os.getFactor(i), proc->getOversamplingFactor());
            BOOST_REQUIRE_EQUAL (factora, factorb);
            BOOST_REQUIRE (proc->getLatencyInSamples() > 0.f);
            BOOST_REQUIRE (os.getLatencySamples(i) > 0.f);
        }
    }

    os.reset();
}

BOOST_AUTO_TEST_SUITE_END()
