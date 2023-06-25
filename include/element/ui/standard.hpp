// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/ui/content.hpp>

namespace element {
class Context;
class MeterBridgeView;
class GuiService;

class StandardContentComponent : public ContentComponent,
                                 public juce::ApplicationCommandTarget {
public:
    StandardContentComponent (Context& ctx);
    ~StandardContentComponent() noexcept;

    void resizeContent (const Rectangle<int>& area) override;

    NavigationConcertinaPanel* getNavigationConcertinaPanel() const { return nav.get(); }

    void setMainView (const String& name);
    void setSecondaryView (const String& name);
    String getMainViewName() const;
    String getAccessoryViewName() const;

    void nextMainView();
    void backMainView();

    void saveState (PropertiesFile*) override;
    void restoreState (PropertiesFile*) override;

    int getNavSize();

    bool isVirtualKeyboardVisible() const;
    void setVirtualKeyboardVisible (const bool isVisible);
    void toggleVirtualKeyboard();
    VirtualKeyboardView* getVirtualKeyboardView() const;

    void setNodeChannelStripVisible (const bool isVisible) override;
    bool isNodeChannelStripVisible() const override;

    void setMeterBridgeVisible (bool);
    bool isMeterBridgeVisible() const;

    void setCurrentNode (const Node& node) override;
    void stabilize (const bool refreshDataPathTrees = false) override;
    void stabilizeViews() override;

    void setShowAccessoryView (const bool show);
    bool showAccessoryView() const;

    // Drag and drop
    bool isInterestedInFileDrag (const StringArray& files) override;
    void filesDropped (const StringArray& files, int x, int y) override;

    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override;
    void itemDropped (const SourceDetails& dragSourceDetails) override;

    void getSessionState (String&) override;
    void applySessionState (const String&) override;

    void presentView (std::unique_ptr<ContentView>) override;
    void setMainView (ContentView* v);

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

    friend class GuiService;
    ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands (Array<CommandID>& commands) override;
    void getCommandInfo (CommandID commandID, ApplicationCommandInfo& result) override;
    bool perform (const InvocationInfo& info) override;
};

using StandardContent = element::StandardContentComponent;

} // namespace element
