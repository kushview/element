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

class SessionGraphsListBox : public ListBox,
                             public ListBoxModel
{
public:
    SessionGraphsListBox (Session* session = nullptr);
    ~SessionGraphsListBox();
    
    int getNumRows() override;
    virtual void paintListBoxItem (int rowNumber, Graphics& g, int width, int height,
                                   bool rowIsSelected) override;

    inline void setSession (Session* s, const bool selectActiveGraph = true)
    {
        session = s;
        updateContent();
        if (session && session->getNumGraphs() > 0 && selectActiveGraph)
            selectRow (session->getActiveGraphIndex());
        else
            selectRow (0);
    }
    
    Node getGraph (int index)   { return session ? session->getGraph (index) : Node(); }
    Node getSelectedGraph()     { return getGraph (getSelectedRow()); }
    
#if 0
    virtual Component* refreshComponentForRow (int rowNumber, bool isRowSelected,
                                               Component* existingComponentToUpdate);
    void listBoxItemClicked (int row, const MouseEvent& ev) override
    virtual void listBoxItemDoubleClicked (int row, const MouseEvent&);
    virtual void backgroundClicked (const MouseEvent&);
    virtual void selectedRowsChanged (int lastRowSelected);
    virtual void deleteKeyPressed (int lastRowSelected);
    virtual void returnKeyPressed (int lastRowSelected);
    virtual void listWasScrolled();
    virtual var getDragSourceDescription (const SparseSet<int>& rowsToDescribe);
    virtual String getTooltipForRow (int row);
    virtual MouseCursor getMouseCursorForRow (int row);
#endif
    
private:
    SessionPtr session;
};

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
    SessionPtr session;
    ValueTree data;

    friend class ValueTree;
    void valueTreePropertyChanged (ValueTree& tree, const Identifier& property) override;
    void valueTreeChildAdded (ValueTree& parent, ValueTree& child) override;
    void valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int indexRomovedAt) override;
    void valueTreeChildOrderChanged (ValueTree& parent, int oldIndex, int newIndex) override;
    void valueTreeParentChanged (ValueTree& tree) override;
    void valueTreeRedirected (ValueTree& tree) override;
};

}
