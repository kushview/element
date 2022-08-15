/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

#include "gui/ContentComponent.h"

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

private:
    ScopedPointer<VirtualKeyboardComponent> keyboard;
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
