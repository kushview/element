#pragma once

#include "ElementApp.h"
#include "session/Node.h"

namespace Element {

class ContentComponent;
class GuiController;
class GraphNode;

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
    Node getNode() const { return node; }
    void restoreAlwaysOnTopState();
    void moved() override;
    void closeButtonPressed() override;
    void resized() override;

    void activeWindowStatusChanged() override;

	int getDesktopWindowStyleFlags() const override
    {
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
