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
        { "Comment",        Colour (0xff3c3c3c) },
        { "Keyword",        Colour (0xff0000cc) },
        { "Operator",       Colour (0xff225500) },
        { "Identifier",     Colour (0xff000000) },
        { "Integer",        Colour (0xff880000) },
        { "Float",          Colour (0xff885500) },
        { "String",         Colour (0xff990099) },
        { "Bracket",        Colour (0xff000055) },
        { "Punctuation",    Colour (0xff004400) }
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
    editor->setTabSize (2, true);
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
