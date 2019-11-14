
#pragma once

#include "gui/LuaConsole.h"

namespace Element {

class LuaConsoleView : public ContentView
{
public:
    LuaConsoleView()
    {
        addAndMakeVisible (console);
    }

    void resized() override
    {
        console.setBounds (getLocalBounds());
    }

private:
    LuaConsoleComponent console;
};

}
