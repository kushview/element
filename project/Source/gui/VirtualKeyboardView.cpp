
#include "engine/AudioEngine.h"
#include "gui/GuiCommon.h"
#include "gui/VirtualKeyboardView.h"

namespace Element
{

VirtualKeyboardView::VirtualKeyboardView()
{
    addAndMakeVisible(keyboard = new MidiKeyboardComponent (
        internalState, MidiKeyboardComponent::horizontalKeyboard));
    setupKeyboard (*keyboard);
}

void VirtualKeyboardView::setupKeyboard (MidiKeyboardComponent& kb)
{
    kb.setKeyWidth (keyWidth);
}

VirtualKeyboardView::~VirtualKeyboardView()
{
    keyboard = nullptr;
}

void VirtualKeyboardView::resized()
{
    if (keyboard)
        keyboard->setBounds (getLocalBounds());
}
    
void VirtualKeyboardView::didBecomeActive()
{
    if (auto* world = ViewHelpers::getGlobals (this))
    {
        if (auto engine = world->getAudioEngine())
        {
            keyboard = new MidiKeyboardComponent (engine->getKeyboardState(), MidiKeyboardComponent::horizontalKeyboard);
            setupKeyboard (*keyboard);
            addAndMakeVisible (keyboard);
        }
    }
}

}
