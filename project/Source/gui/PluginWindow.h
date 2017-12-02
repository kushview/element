#pragma once

#include "ElementApp.h"
#include "session/Node.h"

namespace Element {

class ContentComponent;
class GraphNode;
class GuiController;
class Node;

/** A desktop window containing a plugin's UI. */
class PluginWindow : public DocumentWindow
{
public:
    struct Settings
    {
        Colour backgroundColor;
        int titleBarHeight;
    };
    
    ~PluginWindow();

    ContentComponent* getElementContentComponent() const;
    
    Toolbar* getToolbar() const;
    void updateGraphNode (GraphNode* newNode, Component* newEditor);
    
    void moved();
    void closeButtonPressed();
    void resized();

	int getDesktopWindowStyleFlags() const override {
		return ComponentPeer::windowHasCloseButton |
			   ComponentPeer::windowHasTitleBar;
	}
protected:
    PluginWindow (GuiController&, Component* const uiComp, const Node& node);

private:
    GuiController& gui;
    friend class WindowManager;
    GraphNode* owner;
    Node node;
};

}
