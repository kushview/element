// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/context.hpp>
#include <element/services.hpp>

#include "nodes/scriptnode.hpp"
#include "services/sessionservice.hpp"
#include "ui/scripteditorview.hpp"

namespace element {

BaseScriptEditorView::BaseScriptEditorView()
{
    setName ("BaseScriptEditorView");
    editor.reset (new ScriptEditorComponent (code, &tokens));
    addAndMakeVisible (editor.get());
}

BaseScriptEditorView::~BaseScriptEditorView()
{
    editor.reset (nullptr);
}

void BaseScriptEditorView::resized()
{
    editor->setBounds (getLocalBounds());
}

void BaseScriptEditorView::reset()
{
    code.replaceAllContent (String());
    code.clearUndoHistory();
    code.setSavePoint();
}

void BaseScriptEditorView::reload()
{
    code.replaceAllContent (getScriptContent());
    code.clearUndoHistory();
    code.setSavePoint();
}

//=============================================================================
ScriptEditorView::ScriptEditorView (const Script& s)
    : BaseScriptEditorView(),
      script (s)
{
    setName ("ScriptEditorView");

    saveButton.setButtonText ("Save");
    addAndMakeVisible (saveButton);
    saveButton.onClick = [this]() {
        updateScript();
    };

    reload();
}

ScriptEditorView::~ScriptEditorView()
{
    updateScript();
}

void ScriptEditorView::updateScript()
{
    script.setCode (getCodeDocument().getAllContent());
    getCodeDocument().setSavePoint();
}

String ScriptEditorView::getScriptContent() const
{
    return script.code();
}

void ScriptEditorView::setSaveButtonVisible (bool visible)
{
    saveButton.setVisible (visible);
    resized();
}

void ScriptEditorView::resized()
{
    BaseScriptEditorView::resized();
    if (! saveButton.isVisible())
        return;
    saveButton.changeWidthToFitText (22);
    saveButton.setBounds (
        getWidth() - 16 - saveButton.getWidth(),
        4,
        saveButton.getWidth(),
        saveButton.getHeight());
}

ObservantScriptEditorView::ObservantScriptEditorView (Context& ctx, const Script& s)
    : ScriptEditorView (s)
{
    setSaveButtonVisible (false);
    auto& ss = *ctx.services().find<SessionService>();
    connections.push_back (ss.sigWillSave.connect (
        std::bind (&ScriptEditorView::updateScript, this)));
}

ObservantScriptEditorView::~ObservantScriptEditorView()
{
    for (auto& cn : connections)
        cn.disconnect();
    connections.clear();
}

//=============================================================================
ScriptNodeScriptEditorView::ScriptNodeScriptEditorView (Context& context, const Node& n, bool editUI)
    : BaseScriptEditorView(),
      node (n),
      editingUI (editUI)
{
    setName ("ScriptNodeScriptEditorView");

    auto& ss = *context.services().find<SessionService>();
    connections.push_back (ss.sigWillSave.connect ([this]() {
        updateScript();
    }));

    applyButton.setButtonText (editingUI ? TRANS ("Apply") : TRANS ("Apply"));
    addAndMakeVisible (applyButton);
    applyButton.onClick = [this]() {
        auto r = updateScript();
        if (! r.wasOk())
        {
            AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                              "Script Error",
                                              r.getErrorMessage());
        }
    };

    reload();
}

ScriptNodeScriptEditorView::~ScriptNodeScriptEditorView()
{
    for (auto& c : connections)
        c.disconnect();
    connections.clear();
}

Result ScriptNodeScriptEditorView::updateScript()
{
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

    return r;
}

String ScriptNodeScriptEditorView::getScriptContent() const
{
    if (ScriptNode::Ptr sn = dynamic_cast<ScriptNode*> (node.getObject()))
        return sn->getCodeDocument (editingUI).getAllContent();
    return {};
}

void ScriptNodeScriptEditorView::resized()
{
    BaseScriptEditorView::resized();
    applyButton.changeWidthToFitText (22);
    applyButton.setBounds (
        getWidth() - 16 - applyButton.getWidth(),
        4,
        applyButton.getWidth(),
        applyButton.getHeight());
}

} // namespace element
