/*
    This file is part of Element.
    Copyright (C) 2019-2021  Kushview, LLC.  All rights reserved.
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

#pragma once

#include "engine/nodes/ScriptNode.h"
#include "gui/nodes/NodeEditorComponent.h"
#include "gui/widgets/LuaConsole.h"
#include "gui/LuaTokeniser.h"

namespace Element {

class ScriptingEngine;
class ScriptNodeEditor : public NodeEditorComponent,
                         public ChangeListener
{
public:
    explicit ScriptNodeEditor (ScriptingEngine& scripts, const Node& node);
    ~ScriptNodeEditor() override;

    void resized() override;
    void paint (Graphics&) override;

    void changeListenerCallback (ChangeBroadcaster*) override;

private:
    ScriptingEngine& engine;
    sol::state_view state;
    sol::environment env;
    sol::table widget;
    Component* comp = nullptr;

    TextButton paramsButton;

    PropertyPanel props;
    int propsWidth = 220,
        propsGap = 2;

    SignalConnection portsChangedConnection;
    ScriptNode::Ptr lua;

    FileBrowserComponent fileBrowser;
    std::unique_ptr<FileChooser> chooser;

    void updateAll();
    void updatePreview();
    void updateCodeEditor();
    void updateProperties();
    void updateSize();
    void onPortsChanged();
    sol::table createContext();
    void log (const String& txt) { Logger::writeToLog (txt); }
};

} // namespace Element
