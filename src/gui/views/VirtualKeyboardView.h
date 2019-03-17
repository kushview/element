
#pragma once

#include "gui/ContentComponent.h"

namespace Element
{

class VirtualKeyboardComponent : public MidiKeyboardComponent 
{
public:
    VirtualKeyboardComponent (MidiKeyboardState& s, Orientation o);
    ~VirtualKeyboardComponent() { }

    void setKeypressOctaveOffset (int offset);
    int getKeypressOctaveOffset() const { return keypressOctaveOffset; }

    bool keyPressed (const KeyPress&) override;

private:
    int keypressOctaveOffset = 6;
};

class VirtualKeyboardView : public ContentView
{
public:
    VirtualKeyboardView();
    virtual ~VirtualKeyboardView();
    
    void didBecomeActive() override;
    void stabilizeContent() override { didBecomeActive(); resized(); }

    void paint (Graphics&) override;
    void resized() override;
    bool keyPressed (const KeyPress&, Component*) override;
    bool keyStateChanged (bool) override;

private:
    ScopedPointer<VirtualKeyboardComponent> keyboard;
    MidiKeyboardState internalState;
    int keyWidth = 16;
    
    Slider midiChannel;
    Slider midiProgram;

    void setupKeyboard (VirtualKeyboardComponent&);
};

}
