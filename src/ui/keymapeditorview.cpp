// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/ui/commands.hpp>
#include <element/context.hpp>

#include "ui/guicommon.hpp"
#include "ui/keymapeditorview.hpp"

namespace element {

class KeymapEditor : public KeyMappingEditorComponent
{
public:
    KeymapEditor (KeyPressMappingSet& s) : KeyMappingEditorComponent (s, true)
    {
        disallow.addArray ({ Commands::sessionInsertPlugin });
        readOnly.addArray ({ Commands::copy,
                             Commands::paste,
                             Commands::undo,
                             Commands::redo });
    }

    ~KeymapEditor() {}

    bool shouldCommandBeIncluded (CommandID commandID) override
    {
        return (allow.size() > 0) ? allow.contains (commandID)
                                  : ! disallow.contains (commandID);
    }

    bool isCommandReadOnly (CommandID commandID) override
    {
        return readOnly.contains (commandID);
    }

    String getDescriptionForKeyPress (const KeyPress& key) override
    {
        return KeyMappingEditorComponent::getDescriptionForKeyPress (key);
    }

private:
    Array<CommandID> allow;
    Array<CommandID> disallow;
    Array<CommandID> readOnly;
};

KeymapEditorView::KeymapEditorView()
{
    setName (EL_VIEW_KEYMAP_EDITOR);
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
        if (auto* const mapping = cc->services().find<UI>()->commands().getKeyMappings())
        {
            editor = std::make_unique<KeymapEditor> (*mapping);
            addAndMakeVisible (editor.get());
        }
    }
    resized();
}

void KeymapEditorView::resized()
{
    Rectangle<int> r (getLocalBounds().reduced (2));
    auto r2 = r.removeFromTop (24).reduced (0, 2);
    r2.removeFromRight (4);
    closeButton.changeWidthToFitText();
    const auto idealBtnW = jmax (closeButton.getBestWidthForHeight (24), closeButton.getWidth());
    closeButton.setBounds (r2.removeFromRight (idealBtnW));
    r.removeFromTop (3);
    if (editor != nullptr)
        editor->setBounds (r);
}

void KeymapEditorView::saveMappings()
{
    if (auto* const cc = ViewHelpers::findContentComponent (this))
        if (auto* const mapping = cc->services().find<UI>()->commands().getKeyMappings())
            if (auto xml = mapping->createXml (true))
                cc->context().settings().getUserSettings()->setValue (
                    Settings::keymappingsKey, xml.get());
}

} // namespace element
