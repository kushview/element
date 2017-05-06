
#include "controllers/GuiController.h"
#include "gui/PluginWindow.h"

namespace Element {
    
void GuiController::activate()
{
    Controller::activate();
}

void GuiController::deactivate()
{
    PluginWindow::closeAllCurrentlyOpenWindows();
    Controller::deactivate();
}

}
