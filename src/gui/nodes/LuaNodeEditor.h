/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.
    - Author Michael Fisher <mfisher@kushview.net>

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

#include "engine/nodes/LuaNode.h"
#include <element/ui/nodeeditor.hpp>
#include "gui/LuaTokeniser.h"

namespace element {

class LuaNodeEditor : public NodeEditor,
                      public ChangeListener
{
public:
    explicit LuaNodeEditor (const Node&);
    ~LuaNodeEditor() override;

    void resized() override;
    void paint (Graphics&) override;

    void changeListenerCallback (ChangeBroadcaster*) override;

private:
    CodeDocument document;
    LuaTokeniser tokens;
    std::unique_ptr<CodeEditorComponent> editor;
    TextButton compileButton;
    TextButton editorButton;
    PropertyPanel props;
    SignalConnection portsChangedConnection;
    LuaNode::Ptr lua;

    void updateProperties();
    void onPortsChanged();
};

} // namespace element
