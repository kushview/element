
#pragma once

#include "gui/ContentComponent.h"
#include "gui/LuaConsoleComponent.h"

namespace Element {

class LuaConsoleView : public ContentView
{
public:
    LuaConsoleView()
    {
        setName (EL_VIEW_CONSOLE);
        addAndMakeVisible (console);
    }

    void resized() override
    {
        console.setBounds (getLocalBounds().reduced (4));
    }

private:
    LuaConsoleComponent console;
};

}
