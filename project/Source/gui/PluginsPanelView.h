
#ifndef EL_PLUGINS_PANEL_VIEW_H
#define EL_PLUGINS_PANEL_VIEW_H

#include "ElementApp.h"

namespace Element {
    class PluginManager;
    class PluginTreeView;
    
    class PluginsPanelView : public Component,
                             public ChangeListener
    {
    public:
        PluginsPanelView (PluginManager& pm);
        ~PluginsPanelView();
    
        void resized() override;
        void paint (Graphics&) override;
        
        void changeListenerCallback (ChangeBroadcaster*) override;
        
    private:
        ScopedPointer<PluginTreeView> pluginTree;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginsPanelView);
    };
}

#endif  // EL_PLUGINS_PANEL_VIEW_H
