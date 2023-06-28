/*
    This file is part of Element
    Copyright (C) 2018-2019  Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <element/juce/core.hpp>
#include <element/ui/updater.hpp>
#include <element/version.hpp>

#include <boost/test/unit_test.hpp>

namespace ui = element::ui;
using element::Version;

static const juce::String Updates_xml = R"(
<Updates>
    <ApplicationName>{AnyApplication}</ApplicationName>
    <ApplicationVersion>1.0.0</ApplicationVersion>
    <Checksum>true</Checksum>
    <SHA1>e870f35354eb72af0a68856d64fcfdc7a8a6f09b</SHA1>
    <MetadataName>2023-06-18-2136_meta.7z</MetadataName>
    <PackageUpdate>
        <Name>@0@</Name>
        <DisplayName>Element</DisplayName>
        <Description>Audio Plugin Host</Description>
        <Version>@1@</Version>
        <ReleaseDate>2023-06-18</ReleaseDate>
        <Default>true</Default>
        <Essential>false</Essential>
        <ForcedInstallation>true</ForcedInstallation>
        <RequiresAdminRights>true</RequiresAdminRights>
        <UpdateFile OS="Any" UncompressedSize="45634838" CompressedSize="13337283"/>
        <DownloadableArchives>Element.app.7z</DownloadableArchives>
        <Licenses>
            <License name="License Agreement" file="EULA.txt"/>
        </Licenses>
        <Operations>
            <Operation name="Extract">
                <Argument>/Applications</Argument>
            </Operation>
        </Operations>
        <SHA1>22fd48dc6428c1f7c05dd396e47623446dbb7b72</SHA1>
    </PackageUpdate>
</Updates>
)";

static juce::String makeXml (const juce::String& pkg, const juce::String& vers)
{
    return Updates_xml.replace ("@0@", pkg.trim())
        .replace ("@1@", vers.trim());
}

BOOST_AUTO_TEST_SUITE (UpdateTests)

BOOST_AUTO_TEST_CASE (XML)
{
    juce::XmlDocument doc (makeXml ("net.kushview.element", "1.1.0-0"));
    auto xml = doc.getDocumentElement();
    BOOST_REQUIRE (xml != nullptr);
    BOOST_REQUIRE_MESSAGE (doc.getLastParseError().isEmpty(), doc.getLastParseError().toStdString());
    if (xml == nullptr || doc.getLastParseError().isNotEmpty())
        return;

    for (auto pkg : xml->getChildWithTagNameIterator ("PackageUpdate")) {
        BOOST_REQUIRE_NE (pkg->getChildByName ("Name"), nullptr);
        BOOST_REQUIRE_NE (pkg->getChildByName ("Version"), nullptr);
    }
}

BOOST_AUTO_TEST_CASE (findExe)
{
#if ! defined(__linux__)
    ui::Updater updater;
    BOOST_REQUIRE (updater.exeFile().empty() == false);
#endif
}

BOOST_AUTO_TEST_CASE (GettersSetters)
{
    ui::Updater updater;
    updater.setExeFile ("/home/my/updater.exe");
    BOOST_REQUIRE_EQUAL (updater.exeFile(), std::string ("/home/my/updater.exe"));
    updater.setRepository ("https://cd.kushview.net/element/release/osx");
    BOOST_REQUIRE_EQUAL (updater.repository(), std::string ("https://cd.kushview.net/element/release/osx"));
}

BOOST_AUTO_TEST_CASE (CheckNow)
{
    std::string ID = "net.kushview.element";
    ui::Updater updater (ID, "1.0.0", "https://fakeupdateurl.com");
    updater.setUpdatesXml (makeXml (ID, "1.1.0").toStdString());
    updater.check (false);
    BOOST_REQUIRE_EQUAL (updater.packages().size(), (size_t) 1);
    BOOST_REQUIRE_EQUAL (updater.available().size(), (size_t) 1);

    updater.clear();
    updater.setUpdatesXml (makeXml (ID, "0.1.0").toStdString());
    BOOST_REQUIRE_EQUAL (updater.packages().size(), (size_t) 1);
    BOOST_REQUIRE_EQUAL (updater.available().size(), (size_t) 0);

    updater.clear();
    updater.setUpdatesXml (makeXml (ID + ".sub", "1.1.0").toStdString());
    BOOST_REQUIRE_EQUAL (updater.packages().size(), (size_t) 1);
    BOOST_REQUIRE_EQUAL (updater.available().size(), (size_t) 0);
}

BOOST_AUTO_TEST_CASE (VersionChecks)
{
    std::string v1 = "  1.2.3-4 ";
    std::string v2 = "1.2.3. 4";
    auto segs1 = Version::segments (v1);
    auto segs2 = Version::segments (v2);
    BOOST_REQUIRE (segs1.size() == 4 && segs2.size() == 4);
    if (segs1.size() == 4 && segs2.size() == 4) {
        BOOST_REQUIRE_EQUAL (segs1[0], "1");
        BOOST_REQUIRE_EQUAL (segs1[1], "2");
        BOOST_REQUIRE_EQUAL (segs1[2], "3");
        BOOST_REQUIRE_EQUAL (segs1[3], "4");
        BOOST_REQUIRE_EQUAL (segs2[0], "1");
        BOOST_REQUIRE_EQUAL (segs2[1], "2");
        BOOST_REQUIRE_EQUAL (segs2[2], "3");
        BOOST_REQUIRE_EQUAL (segs2[3], "4");
    }

    BOOST_REQUIRE_EQUAL (Version::toHex (v1),
                         Version::toHex (v2));
    v2 = "1.2.3.5";
    BOOST_REQUIRE_LT (Version::toHex (v1),
                      Version::toHex (v2));

    element::Version ver1 ("1.0.0");
    element::Version ver2 ("1.0.0.0");
    BOOST_REQUIRE (ver1 == ver2);
    BOOST_REQUIRE (ver1 >= ver2);
    BOOST_REQUIRE (ver1 <= ver2);

    ver2 = element::Version ("1.0.0-1");
    BOOST_REQUIRE (ver1 != ver2);
    BOOST_REQUIRE (ver2 >= ver1);
    BOOST_REQUIRE (ver1 <= ver2);
    BOOST_REQUIRE (ver1 < ver2);
    BOOST_REQUIRE (ver2 > ver1);
}

BOOST_AUTO_TEST_SUITE_END()
