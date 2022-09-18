#define BOOST_TEST_MODULE Element
#include <boost/test/included/unit_test.hpp>
#include <element/juce.hpp>
using namespace juce;

struct JuceMessageManagerFixture {
    JuceMessageManagerFixture()
    {
        MessageManager::getInstance();
        juce::initialiseJuce_GUI();
    }

    ~JuceMessageManagerFixture()
    {
        juce::shutdownJuce_GUI();
    }
};

BOOST_GLOBAL_FIXTURE (JuceMessageManagerFixture);

BOOST_AUTO_TEST_SUITE (Element)

BOOST_AUTO_TEST_CASE (Sanity)
{
    BOOST_TEST_REQUIRE (2 + 2 != 5);
}

BOOST_AUTO_TEST_SUITE_END()
