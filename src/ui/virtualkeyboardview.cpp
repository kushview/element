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

#include <element/audioengine.hpp>
#include "ui/guicommon.hpp"
#include "ui/virtualkeyboardview.hpp"

namespace element {

static int getOctaveOffsetForKeyPress (const KeyPress& key, const int fallback = 6)
{
    if (key == KeyPress::numberPad0 || key == '0')
        return 0;
    if (key == KeyPress::numberPad1 || key == '1')
        return 1;
    if (key == KeyPress::numberPad2 || key == '2')
        return 2;
    if (key == KeyPress::numberPad3 || key == '3')
        return 3;
    if (key == KeyPress::numberPad4 || key == '4')
        return 4;
    if (key == KeyPress::numberPad5 || key == '5')
        return 5;
    if (key == KeyPress::numberPad6 || key == '6')
        return 6;
    if (key == KeyPress::numberPad7 || key == '7')
        return 7;
    if (key == KeyPress::numberPad8 || key == '8')
        return 8;
    if (key == KeyPress::numberPad9 || key == '9')
        return 9;

    return fallback;
}

VirtualKeyboardComponent::VirtualKeyboardComponent (MidiKeyboardState& s, Orientation o)
    : MidiKeyboardComponent (s, o)
{
}

void VirtualKeyboardComponent::setKeypressOctaveOffset (int offset)
{
    if (offset < 0)
        offset = 0;
    if (offset > 10)
        offset = 10;
    if (offset == keypressOctaveOffset)
        return;

    keypressOctaveOffset = offset;
    setKeyPressBaseOctave (keypressOctaveOffset);
}

bool VirtualKeyboardComponent::keyPressed (const KeyPress& key)
{
    bool handled = true;
    const auto isShiftDown = key.getModifiers().isShiftDown();

    if (key == KeyPress::numberPadSubtract || key == '-')
    {
        setKeypressOctaveOffset (keypressOctaveOffset - 1);
    }
    else if (key == KeyPress::numberPadAdd
             || (isShiftDown && key == KeyPress::numberPadAdd)
             || (isShiftDown && key == '+'))
    {
        setKeypressOctaveOffset (keypressOctaveOffset + 1);
    }
    else if (key == KeyPress::numberPad0 || key == '0' || key == KeyPress::numberPad1 || key == '1' || key == KeyPress::numberPad2 || key == '2' || key == KeyPress::numberPad3 || key == '3' || key == KeyPress::numberPad4 || key == '4' || key == KeyPress::numberPad5 || key == '5' || key == KeyPress::numberPad6 || key == '6' || key == KeyPress::numberPad7 || key == '7' || key == KeyPress::numberPad8 || key == '8' || key == KeyPress::numberPad9 || key == '9')
    {
        setKeypressOctaveOffset (getOctaveOffsetForKeyPress (key, keypressOctaveOffset));
    }
    else
    {
        handled = MidiKeyboardComponent::keyPressed (key);
    }

    return handled;
}

VirtualKeyboardView::VirtualKeyboardView()
{
    setOpaque (true);
    keyboard = std::make_unique<VirtualKeyboardComponent> (
        internalState, MidiKeyboardComponent::horizontalKeyboard);
    addAndMakeVisible (keyboard.get());
    setupKeyboard (*keyboard);
#if 1

    addAndMakeVisible (midiChannelLabel);
    midiChannelLabel.setFont (Font (12.f));
    midiChannelLabel.setJustificationType (Justification::centredRight);
    midiChannelLabel.setText ("Channel:", dontSendNotification);
    addAndMakeVisible (midiChannel);
    midiChannel.setSliderStyle (Slider::IncDecButtons);
    midiChannel.setRange (1.0, 16.0, 1.0);
    midiChannel.setTextBoxStyle (Slider::TextBoxLeft, false, 30, midiChannel.getTextBoxHeight());
    midiChannel.onValueChange = [this]() {
        keyboard->setMidiChannel (roundToInt (midiChannel.getValue()));
    };

    addAndMakeVisible (midiProgramLabel);
    midiProgramLabel.setFont (Font (12.f));
    midiProgramLabel.setJustificationType (Justification::centredRight);
    midiProgramLabel.setText ("Program:", dontSendNotification);
    addAndMakeVisible (midiProgram);
    midiProgram.setSliderStyle (Slider::IncDecButtons);
    midiProgram.setRange (1.0, 128, 1.0);
    midiProgram.setTextBoxStyle (Slider::TextBoxLeft, false, 34, midiProgram.getTextBoxHeight());
    midiProgram.onValueChange = [this]() {
        auto* const world = ViewHelpers::getGlobals (this);
        AudioEnginePtr engine = world != nullptr ? world->audio() : nullptr;
        if (! engine)
            return;
        auto msg = MidiMessage::programChange (keyboard->getMidiChannel(),
                                               roundToInt (midiProgram.getValue()) - 1);
        msg.setTimeStamp (1.0 + Time::getMillisecondCounterHiRes());
        engine->addMidiMessage (msg, false);
    };

    addAndMakeVisible (sustain);
    sustain.setButtonText ("Sustain");
    sustain.setClickingTogglesState (true);
    sustain.setTriggeredOnMouseDown (true);
    sustain.setColour (TextButton::buttonOnColourId, Colors::toggleBlue);
    sustain.onClick = [this]() {
        auto* const world = ViewHelpers::getGlobals (this);
        AudioEnginePtr engine = world != nullptr ? world->audio() : nullptr;
        if (! engine)
            return;
        engine->addMidiMessage (MidiMessage::controllerEvent (
                                    keyboard->getMidiChannel(), 0x40, sustain.getToggleState() ? 127 : 0)
                                    .withTimeStamp (1.0 + Time::getMillisecondCounterHiRes()));
    };

    addAndMakeVisible (hold);
    hold.setButtonText ("Hold");
    hold.setClickingTogglesState (true);
    hold.setTriggeredOnMouseDown (true);
    hold.setColour (TextButton::buttonOnColourId, Colors::toggleBlue);
    hold.onClick = [this]() {
        auto* const world = ViewHelpers::getGlobals (this);
        AudioEnginePtr engine = world != nullptr ? world->audio() : nullptr;
        if (! engine)
            return;
        engine->addMidiMessage (MidiMessage::controllerEvent (
                                    keyboard->getMidiChannel(), 0x42, hold.getToggleState() ? 127 : 0)
                                    .withTimeStamp (1.0 + Time::getMillisecondCounterHiRes()));
    };

    addAndMakeVisible (widthLabel);
    widthLabel.setFont (Font (12.f));
    widthLabel.setJustificationType (Justification::centredRight);
    widthLabel.setText ("Width:", dontSendNotification);
    addAndMakeVisible (widthDown);
    widthDown.setButtonText ("-");
    widthDown.setConnectedEdges (Button::ConnectedOnRight);
    widthDown.onClick = [this]() {
        keyWidth = jlimit (14, 24, keyWidth - 1);
        stabilizeWidthControls();
    };

    addAndMakeVisible (widthUp);
    widthUp.setButtonText ("+");
    widthUp.setConnectedEdges (Button::ConnectedOnLeft);
    widthUp.onClick = [this]() {
        keyWidth = jlimit (14, 24, keyWidth + 1);
        stabilizeWidthControls();
    };
#endif
}

void VirtualKeyboardView::stabilizeWidthControls()
{
    keyboard->setKeyWidth (keyWidth);
    keyboard->setBlackNoteLengthProportion (keyWidth < 20 ? 0.7 : 0.64);

    widthDown.setEnabled (keyWidth > 14);
    widthUp.setEnabled (keyWidth < 24);
}

void VirtualKeyboardView::saveState (PropertiesFile* props)
{
    props->setValue ("vkChannel", keyboard->getMidiChannel());
    props->setValue ("vkProgram", midiProgram.getValue());
    props->setValue ("vkKeyWidth", keyboard->getKeyWidth());
    props->setValue ("vkBlackLength", keyboard->getBlackNoteLengthProportion());
}

void VirtualKeyboardView::restoreState (PropertiesFile* props)
{
    midiChannel.setValue (props->getDoubleValue ("vkChannel", midiChannel.getValue()),
                          dontSendNotification);
    keyboard->setMidiChannel (roundToInt (midiChannel.getValue()));

    midiProgram.setValue (props->getDoubleValue ("vkProgram", midiProgram.getValue()),
                          dontSendNotification);

    keyboard->setKeyWidth (static_cast<float> (props->getDoubleValue ("vkKeyWidth", (float) keyWidth)));
    keyWidth = jlimit (14, 24, roundToInt (keyboard->getKeyWidth()));
    stabilizeWidthControls();

    keyboard->setBlackNoteLengthProportion (static_cast<float> (
        props->getDoubleValue ("vkBlackLength", keyboard->getBlackNoteLengthProportion())));
}

void VirtualKeyboardView::setupKeyboard (VirtualKeyboardComponent& kb)
{
    kb.setKeyWidth (keyWidth);
}

VirtualKeyboardView::~VirtualKeyboardView()
{
    keyboard = nullptr;
}

void VirtualKeyboardView::paint (Graphics& g)
{
    g.fillAll (Colors::widgetBackgroundColor);
}

void VirtualKeyboardView::resized()
{
    auto r = getLocalBounds();
#if 1
    r.removeFromTop (2);
    auto r2 = r.removeFromTop (18);
    r2.removeFromLeft (4);
    midiChannelLabel.setBounds (r2.removeFromLeft (48));
    midiChannel.setBounds (r2.removeFromLeft (80));
    r2.removeFromLeft (2);
    midiProgramLabel.setBounds (r2.removeFromLeft (52));
    midiProgram.setBounds (r2.removeFromLeft (84));

    r2.removeFromRight (4);
    widthUp.setBounds (r2.removeFromRight (20));
    widthDown.setBounds (r2.removeFromRight (20));
    r2.removeFromRight (2);
    widthLabel.setBounds (r2.removeFromRight (42));

    sustain.changeWidthToFitText (r2.getHeight());
    sustain.setBounds (jmax (midiProgram.getRight() + 2, (getWidth() / 2) - (sustain.getWidth())),
                       r2.getY(),
                       sustain.getWidth(),
                       r2.getHeight());
    hold.setBounds (sustain.getRight(), r2.getY(), sustain.getWidth(), r2.getHeight());

    r.removeFromTop (2);
#endif
    if (keyboard)
        keyboard->setBounds (r);
}

void VirtualKeyboardView::didBecomeActive()
{
    if (keyboardInitialized)
        return;

    if (auto engine = ViewHelpers::getAudioEngine (this))
    {
        keyboard = std::make_unique<VirtualKeyboardComponent> (engine->getKeyboardState(),
                                                               MidiKeyboardComponent::horizontalKeyboard);
        setupKeyboard (*keyboard);
        addAndMakeVisible (keyboard.get());
        resized();
        keyboardInitialized = true;
    }
}

bool VirtualKeyboardView::keyPressed (const KeyPress& key)
{
    if (! keyboard)
        return ContentView::keyPressed (key);

    const auto isShiftDown = key.getModifiers().isShiftDown();
    const auto isCtrlDown = key.getModifiers().isCommandDown();
    const auto isAltDown = key.getModifiers().isAltDown();

    if (! isShiftDown && isCtrlDown && ! isAltDown && (key.isKeyCode ('_') || key.isKeyCode ('-')))
    {
        // ctrl + minus
        midiChannel.setValue (midiChannel.getValue() - 1);
        return true;
    }
    else if (! isShiftDown && isCtrlDown && ! isAltDown && (key.isKeyCode ('=') || key.isKeyCode ('+')))
    {
        // ctrl + plus
        midiChannel.setValue (midiChannel.getValue() + 1);
        return true;
    }

    else if (! isShiftDown && isCtrlDown && isAltDown && (key.isKeyCode ('_') || key.isKeyCode ('-')))
    {
        // ctrl + alt + minus
        midiProgram.setValue (midiProgram.getValue() - 1);
        return true;
    }
    else if (! isShiftDown && isCtrlDown && isAltDown && (key.isKeyCode ('=') || key.isKeyCode ('+')))
    {
        // ctrl + alt + plus
        midiProgram.setValue (midiProgram.getValue() + 1);
        return true;
    }

    else if (isShiftDown && isCtrlDown && isAltDown && (key.isKeyCode ('_') || key.isKeyCode ('-')))
    {
        // shift + ctrl + alt + minus
        if (widthDown.onClick)
            widthDown.onClick();
        return true;
    }
    else if (isShiftDown && isCtrlDown && isAltDown && (key.isKeyCode ('=') || key.isKeyCode ('+')))
    {
        // shift + ctrl + alt + plus
        if (widthUp.onClick)
            widthUp.onClick();
        return true;
    }

    else if (! isShiftDown && isCtrlDown && ! isAltDown && key.isKeyCode (KeyPress::spaceKey))
    {
        // ctrl + spacebar
        sustain.setToggleState (! sustain.getToggleState(), sendNotification);
    }

    else if (! isShiftDown && isCtrlDown && isAltDown && key.isKeyCode (KeyPress::spaceKey))
    {
        // ctrl + alt + spacebar
        hold.setToggleState (! hold.getToggleState(), sendNotification);
    }

    return keyboard->keyPressed (key);
}

bool VirtualKeyboardView::keyStateChanged (bool isDown)
{
    return keyboard != nullptr ? keyboard->keyStateChanged (isDown) : false;
}

void VirtualKeyboardView::visibilityChanged() { didBecomeActive(); }
void VirtualKeyboardView::parentHierarchyChanged() { didBecomeActive(); }

} // namespace element
