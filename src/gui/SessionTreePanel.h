/*
    SessionTreePanel.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "controllers/AppController.h"
#include "controllers/EngineController.h"
#include "gui/ContentComponent.h"
#include "gui/TreeviewBase.h"
#include "gui/ViewHelpers.h"
#include "session/Session.h"

namespace Element {

class SessionTreePanel : public TreePanelBase,
                         private ValueTree::Listener
{
public:
    explicit SessionTreePanel();
    virtual ~SessionTreePanel();

    void refresh();

    void mouseDown (const MouseEvent &event) override;
    void setSession (SessionPtr);
    SessionPtr getSession() const;

    bool keyPressed (const KeyPress&) override;
    
private:
    friend class SessionNodeTreeItem;

    SessionPtr session;
    ValueTree data;
    SignalConnection nodeSelectedConnection;
    
    bool ignoreActiveRootGraphSelectionHandler = false;
    void selectActiveRootGraph();
    
    TreeViewItem* findItemForNode (const Node& node) const;

    void onNodeSelected();

    friend class ValueTree;
    void valueTreePropertyChanged (ValueTree& tree, const Identifier& property) override;
    void valueTreeChildAdded (ValueTree& parent, ValueTree& child) override;
    void valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int indexRomovedAt) override;
    void valueTreeChildOrderChanged (ValueTree& parent, int oldIndex, int newIndex) override;
    void valueTreeParentChanged (ValueTree& tree) override;
    void valueTreeRedirected (ValueTree& tree) override;
};

}
