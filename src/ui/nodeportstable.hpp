// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/ui/content.hpp>
#include <element/node.hpp>

namespace element {

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

} // namespace element
