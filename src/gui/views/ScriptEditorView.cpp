/*
    This file is part of Element.
    Copyright (C) 2021  Kushview, LLC.  All rights reserved.

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

#include "engine/nodes/ScriptNode.h"
#include "gui/views/ScriptEditorView.h"

namespace element {

#if 0
//==============================================================================
class ScriptNodeEditor::CodeEditor : public CodeEditorComponent
{
public:
    CodeEditor (ScriptNodeEditor& o, CodeDocument& doc, CodeTokeniser* tokens)
        : CodeEditorComponent (doc, tokens),
          owner (o)
    {
        setTabSize (4, true);
        setColourScheme (luaColors());
        setFont (getFont().withHeight (getDefaultFontHeight()));
    }

    ~CodeEditor() override {}

    //==========================================================================
    /** Returns the default font height used by this editor */
    float getDefaultFontHeight() const { return defaultFontHeight; }

    //==========================================================================
    void addPopupMenuItems (PopupMenu &menu, const MouseEvent *event) override
    {
        menu.addItem (50001, "Open File");
        menu.addItem (50002, "Save File");
        menu.addSeparator();
        CodeEditorComponent::addPopupMenuItems (menu, event);
    }

    void performPopupMenuAction (int menuItemID) override
    {
        switch (menuItemID)
        {
            case 50001:
            {
                owner.chooser.reset (new FileChooser (
                    "Open script", ScriptManager::getUserScriptsDir(), 
                    "*.lua", false, false, &owner));

                FileChooser& fc (*owner.chooser);
                if (fc.browseForFileToOpen())
                {
                    auto& doc = getDocument();
                    doc.replaceAllContent (
                        fc.getResult().loadFileAsString());
                }
                break;
            }
            
            case 50002:
            {
                 owner.chooser.reset (new FileChooser (
                    "Save script", ScriptManager::getUserScriptsDir(), 
                    "*.lua", false, false, &owner));
                FileChooser& fc (*owner.chooser);
                if (fc.browseForFileToSave (true))
                {
                    TemporaryFile tmpFile (fc.getResult());
                    auto stream = tmpFile.getFile().createOutputStream();
                    if (getDocument().writeToStream (*stream))                
                        tmpFile.overwriteTargetFileWithTemporary();
                }
                break;
            }

            default:
                CodeEditorComponent::performPopupMenuAction (menuItemID);
                break;
        }
    }

private:
    ScriptNodeEditor& owner;
#if JUCE_MAC
    static constexpr float defaultFontHeight = 14.5f;
#elif JUCE_WINDOWS
    static constexpr float defaultFontHeight = 13.f;
#elif JUCE_LINUX
    static constexpr float defaultFontHeight = 16.f;
#else
    static constexpr float defaultFontHeight = 15.f;
#endif
};
#endif

ScriptEditorView::ScriptEditorView()
{
    setName ("ScriptEditorView");
    editor.reset (new ScriptEditorComponent (code, &tokens));
    addAndMakeVisible (editor.get());
}

ScriptEditorView::~ScriptEditorView()
{
    editor.reset (nullptr);
}

void ScriptEditorView::resized()
{
    editor->setBounds (getLocalBounds());
}

void ScriptEditorView::reset()
{
    code.replaceAllContent (String());
    code.clearUndoHistory();
    code.setSavePoint();
}

void ScriptEditorView::reload()
{
    code.replaceAllContent (getScriptContent());
    code.clearUndoHistory();
    code.setSavePoint();
}

void ScriptEditorView::initializeView (ServiceManager&)
{
    reload();
}

//=============================================================================
ScriptNodeScriptEditorView::ScriptNodeScriptEditorView (const Node& n, bool editUI)
    : ScriptEditorView(),
      node (n),
      editingUI (editUI)
{
    setName ("ScriptNodeScriptEditorView");

    applyButton.setButtonText (editingUI ? TRANS ("Apply") : TRANS ("Apply"));
    addAndMakeVisible (applyButton);
    applyButton.onClick = [this]() {
        Result r = Result::ok();

        if (ScriptNode::Ptr sn = dynamic_cast<ScriptNode*> (node.getObject()))
        {
            auto& doc = sn->getCodeDocument (isEditingUI());
            doc.replaceAllContent (getCodeDocument().getAllContent());
            if (isEditingUI())
            {
                sn->sendChangeMessage();
                r = Result::ok();
            }
            else
            {
                r = sn->loadScript (doc.getAllContent());
            }
        }

        if (! r.wasOk())
        {
            AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                              "Script Error",
                                              r.getErrorMessage());
        }
    };
}

String ScriptNodeScriptEditorView::getScriptContent() const
{
    if (ScriptNode::Ptr sn = dynamic_cast<ScriptNode*> (node.getObject()))
        return sn->getCodeDocument (editingUI).getAllContent();
    return {};
}

void ScriptNodeScriptEditorView::resized()
{
    ScriptEditorView::resized();
    applyButton.changeWidthToFitText (22);
    applyButton.setBounds (
        getWidth() - 16 - applyButton.getWidth(),
        4,
        applyButton.getWidth(),
        applyButton.getHeight());
}

} // namespace element
