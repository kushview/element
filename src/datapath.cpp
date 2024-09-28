// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/node.hpp>

#include "appinfo.hpp"
#include "datapath.hpp"

#ifndef EL_INSTALL_DIR_AWARE
#define EL_INSTALL_DIR_AWARE 1
#endif

namespace element {
namespace detail {
inline static StringArray getSubDirs()
{
    auto dirs = StringArray ({ "Controllers",
                               "Graphs",
                               "Nodes",
                               "Scripts",
                               "Sessions" });
    return dirs;
}

inline static void initializeUserLibrary (const File& path)
{
    for (const auto& d : getSubDirs())
    {
        const auto subdir = path.getChildFile (d);
        if (subdir.existsAsFile())
            subdir.deleteFile();
        subdir.createDirectory();
    }
}
} // namespace detail

DataPath::DataPath()
{
    root = defaultUserDataPath();
}

DataPath::~DataPath() {}

const void DataPath::initializeDefaultLocation()
{
    detail::initializeUserLibrary (defaultLocation());
}

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
    return applicationDataDir().getChildFile ("cache/midi/programs");
}

const File DataPath::defaultScriptsDir() { return defaultUserDataPath().getChildFile ("Scripts"); }
const File DataPath::defaultSessionDir() { return defaultUserDataPath().getChildFile ("Sessions"); }
const File DataPath::defaultGraphDir() { return defaultUserDataPath().getChildFile ("Graphs"); }
const File DataPath::defaultControllersDir() { return defaultUserDataPath().getChildFile ("Controllers"); }

juce::File DataPath::getPresetFile (const juce::String& name) const
{
    String path = "Nodes/";
    if (name.isNotEmpty())
        path << name;
    else
        path << String (name.trim().isNotEmpty() ? name : "New Preset");
    path << ".eln";
    return getRootDir().getChildFile (path);
}

File DataPath::createNewPresetFile (const Node& node, const String& name) const
{
    return getPresetFile (node.getName()).getNonexistentSibling();
}

void DataPath::findPresetsFor (const String& format, const String& identifier, NodeArray& nodes) const
{
    const auto presetsDir = getRootDir().getChildFile ("Nodes");
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
    const auto presetsDir = getRootDir().getChildFile ("Nodes");
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
