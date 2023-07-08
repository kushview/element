/*
    This file is part of Element
    Copyright (c) 2019 Kushview, LLC.  All rights reserved.

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

#include <element/node.hpp>

#include "appinfo.hpp"
#include "datapath.hpp"

#ifndef EL_INSTALL_DIR_AWARE
#define EL_INSTALL_DIR_AWARE 1
#endif

namespace element {
namespace DataPathHelpers {
StringArray getSubDirs()
{
    auto dirs = StringArray ({ "Controllers",
                               "Graphs",
                               "Presets",
                               "Scripts",
                               "Sessions" });
    return dirs;
}

void initializeUserLibrary (const File& path)
{
    for (const auto& d : getSubDirs())
    {
        const auto subdir = path.getChildFile (d);
        if (subdir.existsAsFile())
            subdir.deleteFile();
        subdir.createDirectory();
    }
}
} // namespace DataPathHelpers

DataPath::DataPath()
{
    root = defaultUserDataPath();
    DataPathHelpers::initializeUserLibrary (root);
}

DataPath::~DataPath() {}

const File DataPath::applicationDataDir()
{
#if JUCE_MAC
    return File::getSpecialLocation (File::userApplicationDataDirectory)
        .getChildFile ("Application Support")
        .getChildFile (EL_APP_DATA_SUBDIR);
#else
    return File::getSpecialLocation (File::userApplicationDataDirectory)
        .getChildFile (EL_APP_DATA_SUBDIR);
#endif
}

const File DataPath::defaultUserDataPath()
{
    return File::getSpecialLocation (File::userMusicDirectory)
        .getChildFile (EL_APP_NAME);
}

const File DataPath::defaultSettingsFile()
{
    String fn (EL_APP_NAME);
#if JUCE_DEBUG
    fn << "_Debug.conf";
#else
    fn << ".conf";
#endif
    return applicationDataDir().getChildFile (fn);
}

const File DataPath::defaultLocation() { return defaultUserDataPath(); }

const File DataPath::defaultGlobalMidiProgramsDir()
{
    return defaultUserDataPath().getChildFile ("Cache/MIDI/Programs");
}

const File DataPath::defaultScriptsDir() { return defaultUserDataPath().getChildFile ("Scripts"); }
const File DataPath::defaultSessionDir() { return defaultUserDataPath().getChildFile ("Sessions"); }
const File DataPath::defaultGraphDir() { return defaultUserDataPath().getChildFile ("Graphs"); }
const File DataPath::defaultControllersDir() { return defaultUserDataPath().getChildFile ("Controllers"); }

File DataPath::createNewPresetFile (const Node& node, const String& name) const
{
    String path = "Presets/";
    if (name.isNotEmpty())
        path << name;
    else
        path << String (node.getName().isNotEmpty() ? node.getName() : "New Preset");
    path << ".elpreset";
    return getRootDir().getChildFile (path).getNonexistentSibling();
}

void DataPath::findPresetsFor (const String& format, const String& identifier, NodeArray& nodes) const
{
    const auto presetsDir = getRootDir().getChildFile ("Presets");
    if (! presetsDir.exists() || ! presetsDir.isDirectory())
        return;

    for (DirectoryEntry entry : RangedDirectoryIterator (presetsDir, true, EL_PRESET_FILE_EXTENSIONS))
    {
        Node node (Node::parse (entry.getFile()));
        if (node.isValid() && node.getFileOrIdentifier() == identifier && node.getFormat() == format)
        {
            nodes.add (node);
        }
    }
}

void DataPath::findPresetFiles (StringArray& results) const
{
    const auto presetsDir = getRootDir().getChildFile ("Presets");
    if (! presetsDir.exists() || ! presetsDir.isDirectory())
        return;
    for (DirectoryEntry entry : RangedDirectoryIterator (presetsDir, true, EL_PRESET_FILE_EXTENSIONS))
        results.add (entry.getFile().getFullPathName());
}

const File DataPath::installDir()
{
    File dir;
    return dir;
}
} // namespace element
