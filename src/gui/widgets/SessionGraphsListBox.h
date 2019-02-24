
#pragma once

#include "session/Node.h"
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

}
