/*
    This file is part of Element
    Copyright (C) 2014-2022  Kushview, LLC.  All rights reserved.

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

#pragma once

#include <element/juce/core.hpp>

#define EL_PRESET_FILE_EXTENSIONS "*.elp;*.elpreset;"

namespace element {

class Node;
class NodeArray;

class DataPath {
public:
    DataPath();
    ~DataPath();

    /** Returns the app data dir.
        For example, on OSX this is ~/Library/Application Support/Element
     */
    static const juce::File applicationDataDir();

    /** Returns the default settings file */
    static const juce::File defaultSettingsFile();

    /** Returns the default user data path */
    static const juce::File defaultLocation();

    /** Returns the default Node MIDI Presets directory */
    static const juce::File defaultGlobalMidiProgramsDir();

    /** Returns the default User data path */
    static const juce::File defaultUserDataPath();

    /** Returns the default scripts dir (user) */
    static const juce::File defaultScriptsDir();

    /** Returns the default session directory */
    static const juce::File defaultSessionDir();

    /** Returns the default Graph directory */
    static const juce::File defaultGraphDir();

    /** Returns the default Controllers directory */
    static const juce::File defaultControllersDir();

    /** Returns the default Workspaces directory */
    static const juce::File workspacesDir();

    /** Returns the installation directory. May return an invalid file,
        especially when in debug mode. Use this sparingly.
      */
    static const juce::File installDir();

    const juce::File& getRootDir() const { return root; }
    juce::File createNewPresetFile (const Node& node, const juce::String& name = juce::String()) const;
    void findPresetsFor (const juce::String& format, const juce::String& identifier, NodeArray& nodes) const;
    void findPresetFiles (juce::StringArray& results) const;

private:
    juce::File root;
};

} // namespace element
