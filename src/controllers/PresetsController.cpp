
#include "controllers/PresetsController.h"
#include "controllers/GuiController.h"
#include "gui/ContentComponent.h"
#include "session/Session.h"
#include "session/Presets.h"
#include "Globals.h"
#include "DataPath.h"

namespace Element {

struct PresetsController::Pimpl
{
    Pimpl() { }
    ~Pimpl() { }

    void refresh()
    {
    }
};

PresetsController::PresetsController()
{
    pimpl.reset (new Pimpl());
}

PresetsController::~PresetsController()
{
    pimpl.reset (nullptr);
}

void PresetsController::activate()
{ 
}

void PresetsController::deactivate()
{
}

void PresetsController::refresh()
{
    getWorld().getPresetCollection().refresh();
}

void PresetsController::add (const Node& node, const String& presetName)
{
    const DataPath path;
    if (! node.savePresetTo (path, presetName))
    {
        AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, 
            "Preset", "Could not save preset");        
    }
    else
    {
        getWorld().getPresetCollection().refresh();
    }

    if (auto* gui = findSibling<GuiController>())
        if (auto* cc = gui->getContentComponent())
            cc->stabilize (true);
}

}
