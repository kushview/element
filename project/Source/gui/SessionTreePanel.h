/*
    SessionTreePanel.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "gui/TreeviewBase.h"
#include "session/Session.h"

namespace Element {

class SessionGraphsListBox : public ListBox,
                             public ListBoxModel
{
public:
    SessionGraphsListBox (Session* session = nullptr);
    ~SessionGraphsListBox();
    
    int getNumRows() override;
    void paintListBoxItem (int rowNumber, Graphics& g, int width, int height,
                           bool rowIsSelected) override;

    inline void setSession (Session* s)
    {
        session = s;
        updateContent();
    }
    
    Node getSelectedGraph() { return session ? session->getGraph (getSelectedRow()) : Node(); }
    
#if 0
    virtual Component* refreshComponentForRow (int rowNumber, bool isRowSelected,
                                               Component* existingComponentToUpdate);
    virtual void listBoxItemClicked (int row, const MouseEvent&);
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

class SessionTreePanel : public TreePanelBase
{
public:
    explicit SessionTreePanel();
    virtual ~SessionTreePanel();
    void mouseDown (const MouseEvent &event) override;
    void setSession (SessionPtr);
    SessionPtr getSession() const;

private:
    SessionPtr session;
};

}
