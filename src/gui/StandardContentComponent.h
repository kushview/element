/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

#include "gui/ContentComponent.h"

namespace element {

class StandardContentComponent : public ContentComponent
{
public:
    StandardContentComponent (ServiceManager& app);
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
    ScopedPointer<ContentContainer> container;
    StretchableLayoutManager layout;

    class Resizer;
    friend class Resizer;
    ScopedPointer<Resizer> bar1;

    ScopedPointer<VirtualKeyboardView> keyboard;
    ScopedPointer<NodeChannelStripView> nodeStrip;

    bool statusBarVisible;
    int statusBarSize;
    bool toolBarVisible;
    int toolBarSize;
    bool virtualKeyboardVisible = false;
    int virtualKeyboardSize = 80;
    int nodeStripSize = 80;

    String lastMainView;

    void resizerMouseDown();
    void resizerMouseUp();
    void updateLayout();
    void setContentView (ContentView* view, const bool accessory = false);
};

} // namespace element
