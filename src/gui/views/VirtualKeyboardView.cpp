
#include "engine/AudioEngine.h"
#include "gui/GuiCommon.h"
#include "gui/views/VirtualKeyboardView.h"

namespace Element
{

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
{  }

void VirtualKeyboardComponent::setKeypressOctaveOffset (int offset)
{
    if (offset < 0) offset = 0;
    if (offset > 10) offset = 10;
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
    else if (key == KeyPress::numberPad0 || key == '0' ||
             key == KeyPress::numberPad1 || key == '1' ||
             key == KeyPress::numberPad2 || key == '2' ||
             key == KeyPress::numberPad3 || key == '3' ||
             key == KeyPress::numberPad4 || key == '4' ||
             key == KeyPress::numberPad5 || key == '5' ||
             key == KeyPress::numberPad6 || key == '6' ||
             key == KeyPress::numberPad7 || key == '7' ||
             key == KeyPress::numberPad8 || key == '8' ||
             key == KeyPress::numberPad9 || key == '9')
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
    addAndMakeVisible (keyboard = new VirtualKeyboardComponent (
        internalState, MidiKeyboardComponent::horizontalKeyboard));
    setupKeyboard (*keyboard);
   #if 0
    addAndMakeVisible (midiChannel);
    midiChannel.setSliderStyle (Slider::IncDecButtons);
    midiChannel.setRange (1.0, 16.0, 1.0);
    midiChannel.setTextBoxStyle (Slider::TextBoxLeft, false, 60, midiChannel.getTextBoxHeight());
    addAndMakeVisible (midiProgram);
    midiProgram.setSliderStyle (Slider::IncDecButtons);
    midiProgram.setRange (1.0, 128, 1.0);
    midiProgram.setTextBoxStyle (Slider::TextBoxLeft, false, 60, midiProgram.getTextBoxHeight());
   #endif
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
    g.fillAll (LookAndFeel::widgetBackgroundColor);
}

void VirtualKeyboardView::resized()
{
    auto r = getLocalBounds();
   #if 0
    r.removeFromTop (2);
    auto r2 = r.removeFromTop (18);
    midiChannel.setBounds (r2.removeFromLeft (90));
    r2.removeFromLeft (2);
    midiProgram.setBounds (r2.removeFromLeft (90));
    r.removeFromTop (2);
   #endif
    if (keyboard)
        keyboard->setBounds (r);
}

void VirtualKeyboardView::didBecomeActive()
{
    if (auto engine = ViewHelpers::getAudioEngine (this))
    {
        keyboard = new VirtualKeyboardComponent (engine->getKeyboardState(),
            MidiKeyboardComponent::horizontalKeyboard);
        setupKeyboard (*keyboard);
        addAndMakeVisible (keyboard);
    }
}

bool VirtualKeyboardView::keyPressed (const KeyPress& k, Component* c)
{
    return keyboard != nullptr ? keyboard->keyPressed (k) 
        : ContentView::keyPressed (k, c);
}

bool VirtualKeyboardView::keyStateChanged (bool isDown)
{
    return keyboard != nullptr ? keyboard->keyStateChanged (isDown) : false;
}

}
