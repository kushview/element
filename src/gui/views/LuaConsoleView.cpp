
#include "controllers/AppController.h"
#include "scripting/ScriptingEngine.h"
#include "gui/views/LuaConsoleView.h"
#include "Globals.h"

namespace Element {

void LuaConsoleView::initializeView (AppController& app)
{
    auto& e = app.getWorld().getScriptingEngine();
    console.setEnvironment (e.createEnvironment());
}

}
