// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/juce.hpp>
#include <element/script.hpp>

#include "scripting/scriptmanager.hpp"
#include "scripting/bindings.hpp"
#include "datapath.hpp"
#include "sol/sol.hpp"

namespace element {

static void scanForScripts (File dir, Array<ScriptInfo>& results, bool recursive = true)
{
    for (DirectoryEntry entry : RangedDirectoryIterator (dir, recursive, "*.lua"))
    {
        ScriptInfo desc;
        try
        {
            desc = ScriptInfo::parse (entry.getFile());
        } catch (const std::exception& e)
        {
            DBG (e.what());
            desc = {};
        }

        if (desc.valid())
        {
            results.add (desc);
        }
    }
}

static File getDefaultScriptsDir()
{
    return ScriptManager::getSystemScriptsDir();
}

//==============================================================================
class ScriptManager::Registry
{
public:
    Registry (ScriptManager& sm)
        : owner (sm) {}

    void scanDefaults()
    {
        scanDirectory (getDefaultScriptsDir());
    }

    void scanDirectory (const File& dir)
    {
        if (! dir.isDirectory())
            return;

        Array<ScriptInfo> results;
        scanForScripts (dir, results);
        Array<ScriptInfo> newDSP;
        Array<ScriptInfo> newDSPUI;

        for (int i = 0; i < results.size(); ++i)
        {
            const auto d = results[i];
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
    [[maybe_unused]] ScriptManager& owner;
    Array<ScriptInfo> scripts;
    Array<ScriptInfo> dsp, dspui;
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

void ScriptManager::scanDirectory (const File& d)
{
    registry->scanDirectory (d);
}

int ScriptManager::getNumScripts() const
{
    return registry->scripts.size();
}

ScriptInfo ScriptManager::getScript (int index) const
{
    return registry->scripts[index];
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

#if defined(EL_APPIMAGE)
    dir = File::getSpecialLocation (File::currentExecutableFile)
              .getParentDirectory() // bin
              .getParentDirectory() // usr
              .getChildFile ("share/element/scripts");

#elif defined(EL_SCRIPTSDIR)
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
#else
    return {};
#endif
}

File ScriptManager::getUserScriptsDir()
{
    return DataPath::defaultScriptsDir().getFullPathName();
}

} // namespace element
