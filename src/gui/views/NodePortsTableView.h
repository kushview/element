/*
    This file is part of Element
    Copyright (C) 2021  Kushview, LLC.  All rights reserved.

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
#include "session/Node.h"

namespace Element {

class NodePortsTable : public juce::TableListBox,
                       public juce::TableListBoxModel
{
public:
    enum Columns {
        VisibleColumn = 1,
        NameColumn,
        TypeColumn
    };

    NodePortsTable();
    ~NodePortsTable() override;

    //==========================================================================
    void setNode (const Node& newNode);
    Node getNode() const { return node; }

    //==========================================================================
    int getNumRows() override { return node.getNumPorts(); }
    void paintRowBackground (Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell (Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    void cellClicked (int rowNumber, int columnId, const MouseEvent&) override;

#if 0
    virtual Component* refreshComponentForCell (int rowNumber, int columnId, bool isRowSelected,
                                                Component* existingComponentToUpdate);
    virtual void cellClicked (int rowNumber, int columnId, const MouseEvent&);
    virtual void cellDoubleClicked (int rowNumber, int columnId, const MouseEvent&);
    virtual void backgroundClicked (const MouseEvent&);
    virtual void sortOrderChanged (int newSortColumnId, bool isForwards);
    virtual int getColumnAutoSizeWidth (int columnId);
    virtual String getCellTooltip (int rowNumber, int columnId);
    virtual void selectedRowsChanged (int lastRowSelected);
    virtual void deleteKeyPressed (int lastRowSelected);
    virtual void returnKeyPressed (int lastRowSelected);
    virtual void listWasScrolled();
    virtual var getDragSourceDescription (const SparseSet<int>& currentlySelectedRows);
#endif

private:
    Node node;
};

//==============================================================================
class NodePortsTableView : public ContentView
{
public:
    NodePortsTableView();
    ~NodePortsTableView() override;

    void initializeView (AppController&) override {}
    void willBeRemoved() override {}
    void willBecomeActive() override {}
    void didBecomeActive() override {}
    void stabilizeContent() override {}

private:
    class Content; friend class Content;
    std::unique_ptr<Content> content;
};

}
