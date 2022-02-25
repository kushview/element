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
    virtual void paintListBoxItem (int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override;

    inline void setSession (Session* s, const bool selectActiveGraph = true)
    {
        session = s;
        updateContent();
        if (session && session->getNumGraphs() > 0 && selectActiveGraph)
            selectRow (session->getActiveGraphIndex());
        else
            selectRow (0);
    }

    Node getGraph (int index) { return session ? session->getGraph (index) : Node(); }
    Node getSelectedGraph() { return getGraph (getSelectedRow()); }

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

} // namespace Element
