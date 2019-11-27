
#include "controllers/AppController.h"
#include "scripting/LuaEngine.h"
#include "gui/views/LuaConsoleView.h"
#include "Globals.h"

namespace Element {

void LuaConsoleView::initializeView (AppController& app)
{
    auto& e = app.getWorld().getLuaEngine();
    console.setEnvironment (e.createEnvironment());
}

}
