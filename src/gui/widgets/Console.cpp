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

#include "gui/widgets/Console.h"
#include "gui/LookAndFeel.h"

namespace Element {

static void setupEditor (TextEditor& editor)
{
    editor.setFont (Font (Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));
}

class ConsoleBuffer : public TextEditor
{
public:
    ConsoleBuffer()
    {
        setupEditor (*this);
        setReadOnly (true);
        setMultiLine (true, false);
        setReturnKeyStartsNewLine (false);
        setJustification (Justification::bottomLeft);
        setWantsKeyboardFocus (false);
    }
};

//=============================================================================

class ConsolePrompt : public TextEditor
{
public:
    ConsolePrompt()
    {
        setupEditor (*this);
    }

    bool keyPressed (const KeyPress& k) override
    {
        if (k.getKeyCode() == KeyPress::upKey && onUpKey != nullptr)
            return onUpKey();
        else if (k.getKeyCode() == KeyPress::downKey && onDownKey != nullptr)
            return onDownKey();
        return TextEditor::keyPressed (k);
    }

    std::function<bool()> onUpKey;
    std::function<bool()> onDownKey;
};

//=============================================================================

class Console::Content : public Component
{
public:
    Content (Console& o)
        : owner (o)
    {
        addAndMakeVisible (buffer);
        buffer.setLookAndFeel (&style);
        
        addAndMakeVisible (prefixLabel);
        prefixLabel.setText (prefixText, dontSendNotification);
        prefixLabel.setFont (prompt.getFont().withHeight (12.f));
        prefixLabel.setJustificationType (Justification::centred);
        
        addAndMakeVisible (prompt);
        prompt.setLookAndFeel (&style);

        prompt.onUpKey = [this]() -> bool
        {
            DBG("delta: " << historyPos - historyLast);
            
            if (isPositiveAndBelow (historyPos, history.size()))
            {
                prompt.setText (history[historyPos], dontSendNotification);
                prompt.moveCaretToEnd();
            }
            
            historyLast = historyPos;
            historyPos = jmax (0, historyPos - 1);
            
            return true;
        };

        prompt.onDownKey = [this]() -> bool
        {
            DBG("delta: " << historyPos - historyLast);

            if (isPositiveAndBelow (historyPos, history.size()))
            {
                prompt.setText (history[historyPos], dontSendNotification);
                prompt.moveCaretToEnd();
            }
            
            historyLast = historyPos;
            historyPos = jmin (history.size() - 1, historyPos + 1);
            
            return true;
        };

        prompt.onReturnKey = [this]
        {
            auto text = prompt.getText();
            if (text.isEmpty())
                return;
            prompt.setText ({}, dontSendNotification);
            addToHistory (text);
            owner.handleTextEntry (text);
        };
    }

    ~Content()
    {
        buffer.setLookAndFeel (nullptr);
        prompt.setLookAndFeel (nullptr);
    }

    void clear()
    {
        buffer.clear();
        buffer.moveCaretToEnd();
    }

    void addText (const String& text, bool prefix)
    {
        String line = prefix ? prefixText : String();
        if (line.isNotEmpty())
            line << " ";
        line << text;
        
        buffer.moveCaretToEnd();
        buffer.insertTextAtCaret (line.trimEnd());
        buffer.insertTextAtCaret (newLine);
        buffer.moveCaretToEnd();
    }

    void resized() override
    {
        auto r1 = getLocalBounds();
        auto r2 = r1.removeFromBottom (23);
        prefixLabel.setBounds (r2.removeFromLeft (24));
        prompt.setBounds (r2);
        buffer.setBounds (r1);
    }

private:
    Console& owner;
    ConsoleBuffer buffer;
    Label prefixLabel;
    String prefixText { ">" };
    ConsolePrompt prompt;
    StringArray history;
    int historyPos { -1 };
    int historyLast { -1 };

    void addToHistory (const String& text)
    {
        if (history.isEmpty() || (history.size() > 0 && text != history[history.size() - 1]))
           history.add (text);
        if (history.size() > 100)
            history.remove (0);
        historyPos = historyLast = history.size() - 1;
    }

    struct Style : public Element::LookAndFeel
    {
        void fillTextEditorBackground (Graphics& g, int w, int h, TextEditor& e) override
        {
            LookAndFeel::fillTextEditorBackground (g, w, h, e);
        }

        void drawTextEditorOutline (Graphics&, int, int, TextEditor&) override {}

        CaretComponent* createCaretComponent (Component* keyFocusOwner) override
        {
            return new CaretComponent (nullptr);
        }
    } style;
};

Console::Console (const String& name)
{
    setName (name);
    setOpaque (true);
    content.reset (new Content (*this));
    addAndMakeVisible (content.get());
    setSize (100, 100);
}

Console::~Console()
{
    content.reset();
}

void Console::clear()
{
    content->clear();
}

void Console::addText (const String& text, bool prefix)
{
    content->addText (text, prefix);
}

void Console::textEntered (const String& text)
{
    addText (text, true);
}

void Console::resized()
{
    content->setBounds (getLocalBounds());
}

void Console::paint (Graphics& g)
{
    g.fillAll (findColour (Style::contentBackgroundColorId));
}

void Console::handleTextEntry (const String& text)
{
    textEntered (text);
}

}
