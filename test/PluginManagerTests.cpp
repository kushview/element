#include <boost/test/unit_test.hpp>

#include <element/context.hpp>
#include <element/plugins.hpp>

#include "engine/clapprovider.hpp"
#include "utils.hpp"

using namespace element;

BOOST_AUTO_TEST_SUITE (PluginManagerTests)

BOOST_AUTO_TEST_CASE (SupportedFormats)
{
    PluginManager manager;
    for (const auto& supported : Util::compiledAudioPluginFormats())
        BOOST_REQUIRE (! manager.isAudioPluginFormatSupported (supported));

    manager.getNodeFactory().add (new CLAPProvider());
    manager.addDefaultFormats();
    for (const auto& supported : Util::compiledAudioPluginFormats())
        BOOST_REQUIRE_MESSAGE (manager.isAudioPluginFormatSupported (supported), supported.toStdString());
}

BOOST_AUTO_TEST_SUITE_END()
