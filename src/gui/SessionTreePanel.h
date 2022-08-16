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

#include "services.hpp"
#include "services/engineservice.hpp"
#include "gui/ContentComponent.h"
#include "gui/TreeviewBase.h"
#include "gui/ViewHelpers.h"
#include "session/session.hpp"

namespace element {

class SessionTreePanel : public TreePanelBase,
                         private ValueTree::Listener
{
public:
    explicit SessionTreePanel();
    virtual ~SessionTreePanel();

    void refresh();

    void mouseDown (const MouseEvent& event) override;
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

} // namespace element
