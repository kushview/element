/*
    ElementApp.h - This file is part of Element
    Copyright (C) 2014-2018  Kushview, LLC.  All rights reserved.
 */

#pragma once

#include "ElementApp.h"

#define EL_PRESET_FILE_EXTENSIONS "*.elp;*.elpreset"

namespace Element {

class Node;
class NodeArray;

class DataPath
{
public:
    DataPath();
    ~DataPath();
    
    /** Returns the app data dir.
        For example, on OSX this is ~/Library/Application Support/Element */
    static const File applicationDataDir();

    static const File defaultSettingsFile();
    static const File defaultLocation();
    static const File defaultSessionDir();
    static const File defaultGraphDir();
    static const File defaultControllersDir();
    static const File workspacesDir();

    const File& getRootDir() const { return root; }
    File createNewPresetFile (const Node& node, const String& name = String()) const;
    void findPresetsFor (const String& format, const String& identifier, NodeArray& nodes) const;
    void findPresetFiles (StringArray& results) const;
    
private:
    File root;
};

class DataSearchPath { };
}
