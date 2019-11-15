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

#include "gui/LuaConsoleComponent.h"
#include "gui/LookAndFeel.h"
#include "gui/ViewHelpers.h"
#include "scripting/LuaBindings.h"
#include "Commands.h"
#include "sol/sol.hpp"

namespace Element {

static void setupEditor (TextEditor& editor)
{
    editor.setFont (Font (Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));
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
        setWantsKeyboardFocus (false);
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
        buffer.setLookAndFeel (&style);
        addAndMakeVisible (prefix);
        prefix.setText (">", dontSendNotification);
        prefix.setFont (prompt.getFont().withHeight (13.f));
        prefix.setJustificationType (Justification::centred);
        addAndMakeVisible (prompt);
        prompt.setLookAndFeel (&style);

        prompt.onReturnKey = [this]
        {
            auto text = prompt.getText();
            prompt.setText ({}, dontSendNotification);
            buffer.moveCaretToEnd();
            buffer.insertTextAtCaret (String("> ") + text);
            buffer.insertTextAtCaret (newLine);

            buffer.moveCaretToEnd();

            lastError.clear();
            LuaResult result = lua.safe_script (text.toRawUTF8(),
                [this](lua_State* L, LuaResult pfr) { return errorHandler (L, pfr); });
            if (result.valid())
            {
                String str;

                switch (result.get_type())
                {
                    case sol::type::number: str << result.get<double>(); break;
                    case sol::type::string: str << result.get<std::string>(); break;
                    default: break;
                }

                if (str.isNotEmpty())
                {
                    buffer.insertTextAtCaret (str);
                    buffer.insertTextAtCaret (newLine);
                }
            }
            else if (lastError.isNotEmpty())
            {
                buffer.insertTextAtCaret (lastError);
                buffer.insertTextAtCaret (newLine);
            }
        };

        lua.open_libraries();
        Lua::registerUI (lua);
        
        lua["os"]["exit"] = sol::overload (
            [this]()
            {
                ViewHelpers::invokeDirectly (this, Commands::quit, true);
            },
            [this](int code)
            {
                JUCEApplication::getInstance()->setApplicationReturnValue (code);
                ViewHelpers::invokeDirectly (this, Commands::quit, true);
            });

        setSize (100, 100);
    }

    ~Content()
    {
        buffer.setLookAndFeel (nullptr);
        prompt.setLookAndFeel (nullptr);
    }

    void resized() override
    {
        auto r1 = getLocalBounds();
        auto r2 = r1.removeFromBottom (23);
        prefix.setBounds (r2.removeFromLeft (30));
        prompt.setBounds (r2);
        buffer.setBounds (r1);
    }

private:
    using LuaResult = sol::protected_function_result;
    
    LuaConsoleComponent& owner;
    sol::state lua;
    LuaConsoleBuffer buffer;
    Label prefix;
    LuaConsolePrompt prompt;
    String lastError {} ;
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

    LuaResult errorHandler (lua_State* L, LuaResult pfr) {
        lastError = pfr.get<std::string>();
        return pfr;
    }
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
