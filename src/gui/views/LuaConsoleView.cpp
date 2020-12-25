
#include "controllers/AppController.h"
#include "scripting/ScriptingEngine.h"
#include "gui/views/LuaConsoleView.h"
#include "Globals.h"

namespace Element {

void LuaConsoleView::initializeView (AppController& app)
{
    auto& se = app.getWorld().getScriptingEngine();
    console.setEnvironment (
        sol::environment (se.getState(), sol::create, se.getState().globals())
    );
}

}
