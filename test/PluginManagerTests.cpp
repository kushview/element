#include <boost/test/unit_test.hpp>
#include "session/pluginmanager.hpp"
#include "utils.hpp"

using namespace element;

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
