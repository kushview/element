#pragma once

#include "ElementApp.h"

namespace Element {

class ContentComponent;
class GraphNode;
class GuiController;

/** A desktop window containing a plugin's UI. */
class PluginWindow : public DocumentWindow
{
public:
    struct Settings
    {
        Colour backgroundColor;
        int titleBarHeight;
    };
    
    static PluginWindow* getWindowFor (const Node& node);
    static PluginWindow* createWindowFor (const Node& node);
    static PluginWindow* createWindowFor (const Node&, Component* editor);
    static PluginWindow* getOrCreateWindowFor (const Node& node);
    
    static PluginWindow* getWindowFor (GraphNode* node);
    static PluginWindow* createWindowFor (GraphNode* node);
    static PluginWindow* createWindowFor (GraphNode* node, Component* editor);
    static PluginWindow* getOrCreateWindowFor (GraphNode* node);
    static PluginWindow* getFirstWindow();
    
    static void closeCurrentlyOpenWindowsFor (GraphProcessor&, const bool windowVisible = true);
    static void closeCurrentlyOpenWindowsFor (const Node& node, const bool windowVisible = false);
    static void closeCurrentlyOpenWindowsFor (GraphNode* const node, const bool windowVisible = false);
    static void closeCurrentlyOpenWindowsFor (const uint32 nodeId, const bool windowVisible = false);
    static void closeAllCurrentlyOpenWindows (const bool windowVisible = false);

    ~PluginWindow();

    ContentComponent* getElementContentComponent() const;
    
    Toolbar* getToolbar() const;
    void updateGraphNode (GraphNode* newNode, Component* newEditor);
    
    void moved();
    void closeButtonPressed();
    void resized();

private:
    PluginWindow (Component* const uiComp, const Node& node);
    GraphNode* owner;
    Node node;
    
    static void deleteWindow (const int, const bool);
};

}
