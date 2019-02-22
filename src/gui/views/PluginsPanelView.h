
#ifndef EL_PLUGINS_PANEL_VIEW_H
#define EL_PLUGINS_PANEL_VIEW_H

#include "gui/ContentComponent.h"

namespace Element {

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginsPanelView);
};

}

#endif  // EL_PLUGINS_PANEL_VIEW_H
