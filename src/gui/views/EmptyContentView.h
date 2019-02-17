
#pragma once

#include "gui/ContentComponent.h"
#include "gui/LookAndFeel.h"

namespace Element {

class EmptyContentView : public ContentView
{
public:
    EmptyContentView()
    {
        setName ("EmptyView");
    }

    inline void paint (Graphics& g) override
    {
        
        g.fillAll (LookAndFeel::contentBackgroundColor);
        g.setColour (LookAndFeel::textColor);
        g.setFont (16.f);
        
       #if JUCE_MAC
        const String msg ("Session is empty.\nPress Shift+Cmd+N to add a graph.");
       #else
        const String msg ("Session is empty.\nPress Shift+Ctl+N to add a graph.");
       #endif
        g.drawFittedText (msg, 0, 0, getWidth(), getHeight(), Justification::centred, 2);
    }
};

}
