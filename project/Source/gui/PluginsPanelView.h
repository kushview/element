
#ifndef EL_PLUGINS_PANEL_VIEW_H
#define EL_PLUGINS_PANEL_VIEW_H

#include "ElementApp.h"

namespace Element {
    class PluginManager;
    class PluginTreeView;
    
    class PluginsNavigationPanel : public Component,
                                   public ChangeListener
    {
    public:
        PluginsNavigationPanel (PluginManager& pm);
        ~PluginsNavigationPanel();
    
        void resized() override;
        void paint (Graphics&) override;
        
        void changeListenerCallback (ChangeBroadcaster*) override;
    private:
        ScopedPointer<PluginTreeView> pluginTree;
    };
}

#endif  // EL_PLUGINS_PANEL_VIEW_H
