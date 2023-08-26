// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/ui/content.hpp>

namespace element {

class VirtualKeyboardComponent : public MidiKeyboardComponent
{
public:
    VirtualKeyboardComponent (MidiKeyboardState& s, Orientation o);
    ~VirtualKeyboardComponent() {}

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
    void stabilizeContent() override
    {
        didBecomeActive();
        resized();
    }

    void saveState (PropertiesFile*) override;
    void restoreState (PropertiesFile*) override;

    void paint (Graphics&) override;
    void resized() override;
    bool keyPressed (const KeyPress&) override;
    bool keyStateChanged (bool) override;
    void parentHierarchyChanged() override;
    void visibilityChanged() override;

private:
    std::unique_ptr<VirtualKeyboardComponent> keyboard;
    bool keyboardInitialized = false;
    MidiKeyboardState internalState;
    int keyWidth = 16;

    Label midiChannelLabel;
    Slider midiChannel;
    Label midiProgramLabel;
    Slider midiProgram;

    TextButton sustain;
    TextButton hold;

    Label widthLabel;
    TextButton widthDown;
    TextButton widthUp;
    void setupKeyboard (VirtualKeyboardComponent&);
    void stabilizeWidthControls();
};

} // namespace element
