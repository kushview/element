/*
    This file is part of Element
    Copyright (C) 2020  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "JuceHeader.h"
#include "scripting/LuaBindings.h"
#include "scripting/ScriptDescription.h"
#include "scripting/ScriptManager.h"
#include "DataPath.h"
#include "sol/sol.hpp"

namespace Element {

static void scanForScripts (File dir, Array<ScriptDescription>& results, bool recursive = true)
{
    for (DirectoryEntry entry : RangedDirectoryIterator (dir, recursive, "*.lua"))
    {
        ScriptDescription desc;
        try {
            desc = ScriptDescription::parse (entry.getFile());
        } catch (const std::exception& e) {
            DBG (e.what());
            desc = {};
        }

        if (desc.isValid())
        {
            results.add (desc);
        }
    }
}

static File getDefaultScriptsDir()
{
   #if JUCE_DEBUG
    return File::getCurrentWorkingDirectory().getChildFile ("scripts");
   #else
    return {};
   #endif
}

//==============================================================================
class ScriptManager::Registry
{
public:
    Registry (ScriptManager& sm)
        : owner (sm) {}

    void scanDefaults()
    {
        Array<ScriptDescription> results;
        scanForScripts (getDefaultScriptsDir(), results);
        Array<ScriptDescription> newDSP;
        Array<ScriptDescription> newDSPUI;
        
        for (int i = 0; i < results.size(); ++i)
        {
            const auto d = results [i];
            if (d.type.toLowerCase() == "dsp")
            {
                newDSP.add (d);
            }
            else if (d.type.toLowerCase() == "dspui")
            {
                newDSPUI.add (d);
            }
        }
        
        scripts.swapWith (results);
        dsp.swapWith (newDSP);
        dspui.swapWith (newDSPUI);
    }

private:
    friend class ScriptManager;
    ScriptManager& owner;
    Array<ScriptDescription> scripts;
    Array<ScriptDescription> dsp, dspui;
};

//==============================================================================
ScriptManager::ScriptManager()
{
    registry.reset (new Registry (*this));
}

ScriptManager::~ScriptManager() {}

void ScriptManager::scanDefaultLocation()
{
    registry->scanDefaults();
}

int ScriptManager::getNumScripts() const
{
    return registry->scripts.size();
}

ScriptDescription ScriptManager::getScript (int index) const
{
    return registry->scripts [index];
}

const ScriptArray& ScriptManager::getScriptsDSP() const
{
    return registry->dsp;
}


//==============================================================================
File ScriptManager::getApplicationScriptsDir()
{
    return DataPath::applicationDataDir().getChildFile ("Scripts");
}

File ScriptManager::getSystemScriptsDir()
{
    File dir;

   #if defined (EL_APPIMAGE)
    dir = File::getSpecialLocation (File::currentExecutableFile)
        .getParentDirectory() // bin
        .getParentDirectory() // usr
        .getChildFile ("share/element/scripts");

   #elif defined (EL_SCRIPTSDIR)
    if (File::isAbsolutePath (EL_SCRIPTSDIR))
        dir = File (EL_SCRIPTSDIR);

   #elif JUCE_LINUX
    dir = File ("/usr/local/share/element/scripts");

   #elif JUCE_MAC
    dir = File::getSpecialLocation (File::currentApplicationFile)
        .getChildFile ("Contents/Resources/Scripts")
        .getFullPathName();

   #elif JUCE_WINDOWS
    const auto installDir = DataPath::installDir();
    if (installDir.isDirectory())
        dir = installDir.getChildFile ("Scripts");
   #endif
   
    return dir;
}

File ScriptManager::getHomeScriptsDir()
{
   #if JUCE_LINUX || JUCE_MAC
    return File::getSpecialLocation (File::userHomeDirectory)
        .getChildFile (".local/share/element/scripts")
        .getFullPathName();
   #endif
}

File ScriptManager::getUserScriptsDir()
{
    return DataPath::defaultScriptsDir().getFullPathName();
}

}
