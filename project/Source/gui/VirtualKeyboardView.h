
#pragma once

#include "gui/ContentComponent.h"

namespace Element
{

class VirtualKeyboardView : public ContentView
{
public:
    VirtualKeyboardView();
    virtual ~VirtualKeyboardView();
    
    void didBecomeActive() override;
    void resized() override;
    
private:
    ScopedPointer<MidiKeyboardComponent> keyboard;
    MidiKeyboardState internalState;
    
    int keyWidth = 24;
    
    void setupKeyboard (MidiKeyboardComponent&);
};

}
