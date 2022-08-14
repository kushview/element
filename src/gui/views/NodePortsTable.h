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
#include "session/node.hpp"

namespace Element {

//==============================================================================
class NodePortsTable : public juce::Component
{
public:
    NodePortsTable();
    ~NodePortsTable() override;

    void setNode (const Node& node);
    void refresh (int row = -1);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    class TableModel;
    std::unique_ptr<TableModel> model;
    juce::TableListBox table;
    juce::TextButton showAllButton, hideAllButton,
        saveAsDefaultButton;
};

} // namespace Element
