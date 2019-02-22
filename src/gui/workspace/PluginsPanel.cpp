
#if EL_DOCKING

#include "controllers/AppController.h"
#include "gui/views/PluginsPanelView.h"
#include "gui/workspace/PluginsPanel.h"
#include "Globals.h"
#include "session/PluginManager.h"

namespace Element {

PluginsPanel::PluginsPanel() {}

void PluginsPanel::initializeView (AppController& app)
{
    if (view != nullptr)
        return;
    view.reset (new PluginsPanelView (app.getWorld().getPluginManager()));
    addAndMakeVisible (view.get());
}

void PluginsPanel::didBecomeActive() {}
void PluginsPanel::stabilizeContent() {}

void PluginsPanel::resized() 
{
    if (view)
        view->setBounds (getLocalBounds());
}

}

#endif
