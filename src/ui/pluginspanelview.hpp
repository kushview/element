// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#ifndef EL_PLUGINS_PANEL_VIEW_H
#define EL_PLUGINS_PANEL_VIEW_H

#include <element/ui/content.hpp>

namespace element {

class PluginManager;

class PluginsPanelView : public ContentView,
                         public ChangeListener,
                         public TextEditor::Listener,
                         public Timer
{
public:
    PluginsPanelView (PluginManager& pm);
    ~PluginsPanelView();

    void resized() override;
    void paint (Graphics&) override;

    /** Returns the text in the search box */
    String getSearchText() { return search.getText(); }

    /** @internal */
    void textEditorTextChanged (TextEditor&) override;
    void textEditorReturnKeyPressed (TextEditor&) override;
    void changeListenerCallback (ChangeBroadcaster*) override;
    void timerCallback() override;

private:
    PluginManager& plugins;
    TreeView tree;
    TextEditor search;

    void updateTreeView();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginsPanelView);
};

} // namespace element

#endif // EL_PLUGINS_PANEL_VIEW_H
