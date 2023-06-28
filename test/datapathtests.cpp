
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
    params.folderName = params.folderName.replace ("\\", "/");
    const auto fullPath = DataPath::applicationDataDir().getFullPathName().replace ("\\", "/");

    BOOST_REQUIRE_MESSAGE (params.folderName.endsWith (EL_APP_DATA_SUBDIR),
                           params.folderName.toStdString());
    BOOST_REQUIRE_MESSAGE (fullPath.endsWith (EL_APP_DATA_SUBDIR), fullPath.toStdString());
    BOOST_REQUIRE_EQUAL (DataPath::defaultSettingsFile().getFullPathName().toStdString(),
                         s.getUserSettings()->getFile().getFullPathName().toStdString());
}

BOOST_AUTO_TEST_SUITE_END()
