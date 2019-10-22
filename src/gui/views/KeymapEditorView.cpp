/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

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

#include "gui/GuiCommon.h"
#include "session/CommandManager.h"
#include "Globals.h"
#include "gui/views/KeymapEditorView.h"

namespace Element  {

class KeymapEditor : public KeyMappingEditorComponent
{
public:
    KeymapEditor (KeyPressMappingSet& s) : KeyMappingEditorComponent (s, true)
    {
        disallow.addArray ({
            Commands::exportAudio, Commands::exportMidi,
            Commands::mediaClose, Commands::mediaNew, Commands::mediaOpen, 
            Commands::mediaSave, Commands::mediaSaveAs,
            Commands::sessionInsertPlugin,
            Commands::signIn, Commands::signOut,
           #if defined (EL_PRO) && EL_DOCKING
            Commands::workspaceClassic, Commands::workspaceEditing,
            Commands::workspaceOpen, Commands::workspaceResetActive,
            Commands::workspaceSave, Commands::workspaceSaveActive
           #endif
        });

        readOnly.addArray ({
            Commands::copy, Commands::paste, 
            Commands::undo, Commands::redo
        });
    }

    ~KeymapEditor() { }

    bool shouldCommandBeIncluded (CommandID commandID) override
    {
        return (allow.size() > 0) ? allow.contains (commandID)
                                  : !disallow.contains (commandID);
    }

    bool isCommandReadOnly (CommandID commandID) override {
        return readOnly.contains (commandID);
    }

    String getDescriptionForKeyPress (const KeyPress &key) override {
        return KeyMappingEditorComponent::getDescriptionForKeyPress (key);
    }

private:
    Array<CommandID> allow;
    Array<CommandID> disallow;
    Array<CommandID> readOnly;
};

KeymapEditorView::KeymapEditorView()
{ 
    setName ("KeymapEditorView");
    addAndMakeVisible (closeButton);
    closeButton.setButtonText (TRANS ("Close"));
    closeButton.setSize (24, 24);
    closeButton.changeWidthToFitText (24);
    closeButton.addListener (this);
}

void KeymapEditorView::buttonClicked (Button* b)
{
    saveMappings();
    ViewHelpers::invokeDirectly (this, Commands::showLastContentView, true);
}

void KeymapEditorView::stabilizeContent()
{
    editor = nullptr;
    if (auto* const cc = ViewHelpers::findContentComponent (this))
    {
        auto* const mapping = cc->getGlobals().getCommandManager().getKeyMappings();
        addAndMakeVisible (editor = new KeymapEditor (*mapping));
    }
    resized();
}

void KeymapEditorView::resized()
{
    Rectangle<int> r (getLocalBounds().reduced (2));
    auto r2 = r.removeFromTop(24).reduced (0, 2);
    r2.removeFromRight (4);
    closeButton.changeWidthToFitText();
    const auto idealBtnW = jmax (closeButton.getBestWidthForHeight(24), closeButton.getWidth());
    closeButton.setBounds (r2.removeFromRight (idealBtnW));
    r.removeFromTop (3);
    if (editor != nullptr)
        editor->setBounds (r);
}

void KeymapEditorView::saveMappings()
{
    if (auto* const cc = ViewHelpers::findContentComponent (this))
        if (auto* const mapping = cc->getGlobals().getCommandManager().getKeyMappings())
            if (ScopedPointer<XmlElement> xml = mapping->createXml (false))
                cc->getGlobals().getSettings().getUserSettings()->setValue (
                    "keymappings", xml.get());
}

}
