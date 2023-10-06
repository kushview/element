// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

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

    static const void initializeDefaultLocation();

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
