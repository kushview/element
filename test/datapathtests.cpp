
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
#if ! ELEMENT_APPIMAGE
    Settings s;
    auto params = s.getStorageParameters();
    params.folderName = params.folderName.replace ("\\", "/");
    const auto fullPath = DataPath::applicationDataDir().getFullPathName().replace ("\\", "/");

    BOOST_REQUIRE_MESSAGE (params.folderName.endsWith (EL_APP_DATA_SUBDIR),
                           params.folderName.toStdString());
    BOOST_REQUIRE_MESSAGE (fullPath.endsWith (EL_APP_DATA_SUBDIR), fullPath.toStdString());
    // Some CI systems, like the archlinux docker container running on github, might have double
    // slashes in the settings file path... e.g. when the $HOME is blank.
    BOOST_REQUIRE_EQUAL (
        DataPath::defaultSettingsFile().getFullPathName().replace("//", "/").toStdString(),
        s.getUserSettings()->getFile().getFullPathName().replace("//", "/").toStdString());
#else
    BOOST_REQUIRE (true);
#endif
}

BOOST_AUTO_TEST_SUITE_END()
