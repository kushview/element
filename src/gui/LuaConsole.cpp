/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "gui/LuaConsole.h"

namespace Element {

static void setupEditor (TextEditor& editor)
{
    editor.setFont (Font (Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
}

class LuaConsoleBuffer : public TextEditor
{
public:
    LuaConsoleBuffer()
    {
        setupEditor (*this);
        setReadOnly (true);
        setMultiLine (true, false);
        setReturnKeyStartsNewLine (false);
        setJustification (Justification::bottomLeft);
    }
};

//=============================================================================

class LuaConsolePrompt : public TextEditor
{
public:
    LuaConsolePrompt()
    {
        setupEditor (*this);
    }
};

//=============================================================================

class LuaConsoleComponent::Content : public Component
{
public:
    Content (LuaConsoleComponent& o)
        : owner (o)
    {
        addAndMakeVisible (buffer);
        addAndMakeVisible (prompt);

        prompt.onReturnKey = [this]
        {
            auto text = prompt.getText().fromFirstOccurrenceOf ("> ", false, false);
            prompt.setText ("> ", dontSendNotification);
            buffer.moveCaretToEnd();
            buffer.insertTextAtCaret (String("> ") + text);
            buffer.insertTextAtCaret (newLine);
        };

        setSize (100, 100);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        prompt.setBounds (r.removeFromBottom (23));
        buffer.setBounds (r);
    }

private:
    LuaConsoleComponent& owner;
    LuaConsoleBuffer buffer;
    LuaConsolePrompt prompt;
};

LuaConsoleComponent::LuaConsoleComponent()
{
    setName ("Lua Console");
    setOpaque (true);
    content.reset (new Content (*this));
    addAndMakeVisible (content.get());
    setSize (100, 100);
}

LuaConsoleComponent::~LuaConsoleComponent()
{
    content.reset();
}

void LuaConsoleComponent::resized()
{
    content->setBounds (getLocalBounds());
}

void LuaConsoleComponent::paint (Graphics& g)
{
    g.fillAll (Colours::black);
}

}
