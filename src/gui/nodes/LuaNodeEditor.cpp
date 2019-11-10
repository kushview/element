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

LuaNodeEditor::LuaNodeEditor (const Node& node)
    : NodeEditorComponent (node)
{
    setOpaque (true);
    editor.reset (new CodeEditorComponent (document, &tokens));
    addAndMakeVisible (editor.get());
    if (auto* const lua = getNodeObjectOfType<LuaNode>())
        editor->loadContent (lua->getDraftScript());
    setSize (475, 340);
}

LuaNodeEditor::~LuaNodeEditor()
{
    if (auto* const lua = getNodeObjectOfType<LuaNode>())
        lua->setDraftScript (document.getAllContent());
}

void LuaNodeEditor::paint (Graphics& g)
{ 
    g.fillAll (Element::LookAndFeel::backgroundColor);
}

void LuaNodeEditor::resized()
{
    editor->setBounds (getLocalBounds().reduced (4));
}

}
