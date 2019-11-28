
#pragma once

#include "gui/ContentComponent.h"
#include "gui/widgets/LuaConsole.h"

namespace Element {

class AppController;

class LuaConsoleView : public ContentView
{
public:
    LuaConsoleView()
    {
        setName (EL_VIEW_CONSOLE);
        addAndMakeVisible (console);
    }

    void initializeView (AppController&) override;
    void didBecomeActive() override
    {
        console.grabKeyboardFocus();
    }
    
    void resized() override
    {
        console.setBounds (getLocalBounds().reduced (2));
    }

private:
    LuaConsole console;
};

}
