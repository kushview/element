#include "gui/views/ScriptEditorView.h"

namespace Element {

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

}
