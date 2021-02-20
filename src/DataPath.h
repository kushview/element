/*
    This file is part of Element
    Copyright (C) 2014-2019  Kushview, LLC.  All rights reserved.

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

    /** Returns the installation directory. May return an invalid file,
        especially when in debug mode. Use this sparingly
      */
    static const File installDir();

    const File& getRootDir() const { return root; }
    File createNewPresetFile (const Node& node, const String& name = String()) const;
    void findPresetsFor (const String& format, const String& identifier, NodeArray& nodes) const;
    void findPresetFiles (StringArray& results) const;
    
private:
    File root;
};

class DataSearchPath { };
}
