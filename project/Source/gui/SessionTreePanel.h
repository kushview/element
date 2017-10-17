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
    void paintListBoxItem (int rowNumber, Graphics& g, int width, int height,
                           bool rowIsSelected) override;

    inline void setSession (Session* s)
    {
        session = s;
        updateContent();
        if (session && session->getNumGraphs() > 0)
            selectRow (0);
    }
    
    Node getSelectedGraph() { return session ? session->getGraph (getSelectedRow()) : Node(); }
    
    void listBoxItemClicked (int row, const MouseEvent& ev) override
    {
        if (ev.mods.isPopupMenu())
            return;
        const Node graph (getSelectedGraph());
        DBG(graph.getValueTree().toXmlString());
//        if (auto* cc = ViewHelpers::findContentComponent(this))
//            if (auto* ec = cc->getAppController().findChild<EngineController>()) {
//                
//                
//                ec->setRootNode (graph);
//            }
    }
#if 0
    virtual Component* refreshComponentForRow (int rowNumber, bool isRowSelected,
                                               Component* existingComponentToUpdate);
    
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
