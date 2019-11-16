
#pragma once

#include "gui/LuaConsoleComponent.h"

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
        console.setBounds (getLocalBounds().reduced (4));
    }

private:
    LuaConsoleComponent console;
};

}
