// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/ui/content.hpp>

namespace element {
class Context;
class MeterBridgeView;

class StandardContentComponent : public ContentComponent {
public:
    StandardContentComponent (Context& ctx);
    ~StandardContentComponent() noexcept;

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

    bool isVirtualKeyboardVisible() const override;
    void setVirtualKeyboardVisible (const bool isVisible) override;
    void toggleVirtualKeyboard() override;
    VirtualKeyboardView* getVirtualKeyboardView() const override;

    void setNodeChannelStripVisible (const bool isVisible) override;
    bool isNodeChannelStripVisible() const override;

    void setMeterBridgeVisible (bool) override;
    bool isMeterBridgeVisible() const override;

    void setCurrentNode (const Node& node) override;
    void stabilize (const bool refreshDataPathTrees = false) override;
    void stabilizeViews() override;

    void setShowAccessoryView (const bool show) override;
    bool showAccessoryView() const override;

    // Drag and drop
    bool isInterestedInFileDrag (const StringArray& files) override;
    void filesDropped (const StringArray& files, int x, int y) override;

    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override;
    void itemDropped (const SourceDetails& dragSourceDetails) override;

    void getSessionState (String&) override;
    void applySessionState (const String&) override;

    void setMainView (ContentView* v) override;

    // App commands
    void getAllCommands (Array<CommandID>&) override {}
    void getCommandInfo (CommandID, ApplicationCommandInfo&) override {}
    bool perform (const InvocationInfo&) override;
    ApplicationCommandTarget* getNextCommandTarget() override;

private:
    ScopedPointer<NavigationConcertinaPanel> nav;
    friend class ContentContainer;
    ScopedPointer<ContentContainer> container;
    StretchableLayoutManager layout;
    class Resizer;
    friend class Resizer;
    ScopedPointer<Resizer> bar1;

    ScopedPointer<NodeChannelStripView> nodeStrip;

    bool statusBarVisible { true };
    int statusBarSize;
    bool toolBarVisible { true };
    int toolBarSize;
    int virtualKeyboardSize = 80;
    int nodeStripSize = 80;

    String lastMainView;

    void resizerMouseDown();
    void resizerMouseUp();
    void updateLayout();
    void setContentView (ContentView* view, const bool accessory = false);
};

using StandardContent = element::StandardContentComponent;

} // namespace element
