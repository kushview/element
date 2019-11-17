/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.
    - Author Michael Fisher <mfisher@kushvie.net>

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

#include "engine/nodes/LuaNode.h"
#include "gui/nodes/LuaNodeEditor.h"
#include "gui/LookAndFeel.h"

namespace Element {

static CodeEditorComponent::ColourScheme luaColors()
{
    static const CodeEditorComponent::ColourScheme::TokenType types[] =
    {
         { "Error",          Colour (0xffcc0000) },
         { "Comment",        Colour (0xff6a9955) },
         { "Keyword",        Colour (0xff569cd6) },
         { "Operator",       Colour (0xffb3b3b3) },
         { "Identifier",     Colour (0xffc5c5c5) },
         { "Integer",        Colour (0xffb5cea8) },
         { "Float",          Colour (0xffb5cea8) },
         { "String",         Colour (0xffce9178) },
         { "Bracket",        Colour (0xffd4d4d4) },
         { "Punctuation",    Colour (0xffb3b3b3) },
         { "Preprocessor Text", Colour (0xffc586c0) } // used for control statements
    };

    CodeEditorComponent::ColourScheme cs;

    for (auto& t : types)
        cs.set (t.name, Colour (t.colour));

    return cs;
}

LuaNodeEditor::LuaNodeEditor (const Node& node)
    : NodeEditorComponent (node)
{
    auto* const lua = getNodeObjectOfType<LuaNode>();
    jassert(lua);

    setOpaque (true);
    editor.reset (new CodeEditorComponent (document, &tokens));
    addAndMakeVisible (editor.get());
    editor->setTabSize (3, true);
    editor->setFont (editor->getFont().withHeight (15));
    editor->loadContent (lua->getDraftScript());
    editor->setColourScheme (luaColors());
    
    addAndMakeVisible (compileButton);
    compileButton.setButtonText ("Compile");
    compileButton.onClick = [this]()
    {
        if (auto* const lua = getNodeObjectOfType<LuaNode>())
        {
            const auto script = document.getAllContent();
            auto result = lua->loadScript (script);
            if (! result.wasOk())
            {
                AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                    "Script Error", result.getErrorMessage());
            }
        }
    };

    lua->addChangeListener (this);
    setSize (660, 480);
}

LuaNodeEditor::~LuaNodeEditor()
{
    if (auto* const lua = getNodeObjectOfType<LuaNode>())
    {
        lua->removeChangeListener (this);
        lua->setDraftScript (document.getAllContent());
    }
}

void LuaNodeEditor::changeListenerCallback (ChangeBroadcaster*)
{
    if (auto* const lua = getNodeObjectOfType<LuaNode>())
        editor->loadContent (lua->getDraftScript());
    resized();
}

void LuaNodeEditor::paint (Graphics& g)
{ 
    g.fillAll (Element::LookAndFeel::backgroundColor);
}

void LuaNodeEditor::resized()
{
    auto r1 = getLocalBounds().reduced (4);
    auto r2 = r1.removeFromTop (22);
    compileButton.changeWidthToFitText (r2.getHeight());
    compileButton.setBounds (r2.removeFromRight (compileButton.getWidth()));
    r1.removeFromTop (2);
    editor->setBounds (r1);
}

}
