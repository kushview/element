#include <boost/test/unit_test.hpp>
#include "session/PluginManager.h"
#include "utils.hpp"

using namespace Element;

BOOST_AUTO_TEST_SUITE (PluginManagerTests)

BOOST_AUTO_TEST_CASE (SupportedFormats)
{
    PluginManager manager;
    for (const auto& supported : Util::getSupportedAudioPluginFormats())
        BOOST_REQUIRE (! manager.isAudioPluginFormatSupported (supported));
    
    manager.addDefaultFormats();
    for (const auto& supported : Util::getSupportedAudioPluginFormats())
        BOOST_REQUIRE (manager.isAudioPluginFormatSupported (supported));
}

BOOST_AUTO_TEST_SUITE_END()
