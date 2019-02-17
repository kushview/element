/*
    ContentComponentSolo.h - This file is part of Element
    Copyright (c) 2016-2019 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "gui/ContentComponent.h"

namespace Element {

class ContentComponentSolo : public ContentComponent
{
public:
    ContentComponentSolo (AppController& app);
    ~ContentComponentSolo() noexcept;

    void resizeContent (const Rectangle<int>& area) override;

    NavigationConcertinaPanel* getNavigationConcertinaPanel() const override { return nav.get(); }
    
    void setMainView (const String& name) override;
    void setAccessoryView (const String& name) override;
    String getMainViewName() const override;
    String getAccessoryViewName() const override;

    void nextMainView() override;
    void backMainView() override;
    
    void saveState (PropertiesFile*) override;
    void restoreState (PropertiesFile*) override;
    
    int getNavSize() override;
    
    bool isVirtualKeyboardVisible() const override { return virtualKeyboardVisible; }
    void setVirtualKeyboardVisible (const bool isVisible) override;
    void toggleVirtualKeyboard() override;
    VirtualKeyboardView* getVirtualKeyboardView() const override { return keyboard.get(); }
    
    void setNodeChannelStripVisible (const bool isVisible) override;
    bool isNodeChannelStripVisible() const override;

    void setCurrentNode (const Node& node) override;
    void stabilize (const bool refreshDataPathTrees = false) override;
    void stabilizeViews() override;
    
    void setShowAccessoryView (const bool show) override;
    bool showAccessoryView() const override;

    // Drag and drop
    bool isInterestedInFileDrag (const StringArray &files) override;
    void filesDropped (const StringArray &files, int x, int y) override;
    
    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override;
    void itemDropped (const SourceDetails& dragSourceDetails) override;

    // App commands
    void getAllCommands (Array<CommandID>&) override { }
    void getCommandInfo (CommandID, ApplicationCommandInfo&) override { }
    bool perform (const InvocationInfo&) override { return false; }
    ApplicationCommandTarget* getNextCommandTarget() override;

private:
    ScopedPointer<NavigationConcertinaPanel> nav;
    ScopedPointer<ContentContainer> container;
    StretchableLayoutManager layout;
    
    class Resizer; friend class Resizer;
    ScopedPointer<Resizer> bar1;
        
    ScopedPointer<VirtualKeyboardView> keyboard;
    ScopedPointer<NodeChannelStripView> nodeStrip;

    bool statusBarVisible;
    int statusBarSize;
    bool toolBarVisible;
    int toolBarSize;
    bool virtualKeyboardVisible = false;
    int virtualKeyboardSize = 80;
    int nodeStripSize = 90;
    
    String lastMainView;
    
    void resizerMouseDown();
    void resizerMouseUp();
    void updateLayout();
    void setContentView (ContentView* view, const bool accessory = false);
};

}
