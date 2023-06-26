
#include <boost/test/unit_test.hpp>

#include <element/juce/data_structures.hpp>

#include <element/datapath.hpp>
#include <element/settings.hpp>
#include "appinfo.hpp"

using element::DataPath;
using element::Settings;

BOOST_AUTO_TEST_SUITE (DataPathTests)

BOOST_AUTO_TEST_CASE (PathsMatch)
{
    Settings s;
    auto params = s.getStorageParameters();
    BOOST_REQUIRE (params.folderName.endsWith (EL_APP_DATA_SUBDIR));
    BOOST_REQUIRE (DataPath::applicationDataDir().getFullPathName().endsWith (EL_APP_DATA_SUBDIR));
    BOOST_REQUIRE_EQUAL (DataPath::defaultSettingsFile().getFullPathName().toStdString(),
                         s.getUserSettings()->getFile().getFullPathName().toStdString());
}

BOOST_AUTO_TEST_SUITE_END()
