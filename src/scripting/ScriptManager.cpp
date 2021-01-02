#include "JuceHeader.h"
#include "scripting/LuaBindings.h"
#include "scripting/ScriptDescription.h"
#include "scripting/ScriptManager.h"
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

}
