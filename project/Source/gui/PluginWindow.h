#ifndef ELEMENT_PLUGIN_WINDOW_H
#define ELEMENT_PLUGIN_WINDOW_H

#include "element/Juce.h"

namespace Element {

/** A desktop window containing a plugin's UI. */
class PluginWindow : public DocumentWindow
{
public:
    struct Settings
    {
        Colour backgroundColor;
        int titleBarHeight;
    };
    
    static PluginWindow* getWindowFor (GraphNode* node);
    static PluginWindow* createWindowFor (GraphNode* node);
    static PluginWindow* createWindowFor (GraphNode* node, Component* editor);
    static PluginWindow* getOrCreateWindowFor (GraphNode* node);
    
    static void closeCurrentlyOpenWindowsFor (GraphNode* const node);
    static void closeCurrentlyOpenWindowsFor (const uint32 nodeId);
    static void closeAllCurrentlyOpenWindows();

    ~PluginWindow();

    Toolbar* getToolbar() const;
    
    // component/document window
    void moved();
    void closeButtonPressed();
    void resized();

private:
    PluginWindow (Component* const uiComp, GraphNode* node);
    GraphNode* owner;

};

}


#endif // ELEMENT_PLUGIN_WINDOW_H
