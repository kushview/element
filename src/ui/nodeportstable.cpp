#include <element/ui/style.hpp>

#include "ui/nodeportstable.hpp"
#include "ui/viewhelpers.hpp"

namespace element {

//==============================================================================
class NodePortsTable::TableModel : public juce::TableListBoxModel
{
public:
    enum Columns
    {
        VisibleColumn = 1,
        NameColumn,
        TypeColumn
    };

    TableModel (NodePortsTable& ot)
        : table (ot) {}
    ~TableModel() {}

    //==========================================================================
    void setNode (const Node& newNode)
    {
        node = newNode;
    }

    Node getNode() const { return node; }

    //==========================================================================
    int getNumRows() override { return node.getNumPorts(); }

    void paintRowBackground (Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override
    {
        ignoreUnused (rowNumber);
        ViewHelpers::drawBasicTextRow (String(), g, width, height, rowIsSelected);
    }

    void paintCell (Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override
    {
        String text;
        const auto port = node.getPort (rowNumber);

        switch (columnId)
        {
            case VisibleColumn: {
                text = {};
                float sz = (float) jmin (width, height);
                Rectangle<float> r (0.0, 0.0, sz, sz);
                r.reduce (3, 3);
                table.getLookAndFeel().drawTickBox (
                    g, table, r.getX(), r.getY(), r.getWidth(), r.getHeight(), ! port.isHiddenOnBlock(), true, false, false);
                break;
            }

            case NameColumn: {
                text = port.getName();
                break;
            }

            case TypeColumn: {
                text = port.getType().getName() + String (port.isInput() ? " In" : " Out");
                break;
            }
        }

        if (text.isNotEmpty())
        {
            g.setColour (Colours::grey);
            g.setFont (Font (height * 0.7f));
            g.drawFittedText (text, 4, 0, width - 6, height, Justification::centredLeft, 1, 0.9f);
        }
    }

    void cellClicked (int rowNumber, int columnId, const MouseEvent&) override
    {
        if (columnId != VisibleColumn)
            return;
        auto port = node.getPort (rowNumber);
        port.setHiddenOnBlock (! port.isHiddenOnBlock());
        table.refresh (rowNumber);
    }

private:
    Node node;
    NodePortsTable& table;
};

//==========================================================================

//==============================================================================
NodePortsTable::NodePortsTable()
{
    setOpaque (true);

    model.reset (new TableModel (*this));
    table.setModel (model.get());
    auto& header = table.getHeader();
    header.addColumn ("", TableModel::VisibleColumn, 30, 30, 30, TableHeaderComponent::defaultFlags);
    header.addColumn ("Name", TableModel::NameColumn, 100, 30, -1, TableHeaderComponent::defaultFlags);
    header.addColumn ("Type", TableModel::TypeColumn, 100, 30, -1, TableHeaderComponent::defaultFlags);
    addAndMakeVisible (table);

    addAndMakeVisible (showAllButton);
    showAllButton.setButtonText (TRANS ("Show all"));
    showAllButton.onClick = [this]() {
        const auto node = model->getNode();
        for (int i = 0; i < node.getNumPorts(); ++i)
            node.getPort (i).setHiddenOnBlock (false);
        table.updateContent();
        table.repaint();
    };

    addAndMakeVisible (hideAllButton);
    hideAllButton.setButtonText (TRANS ("Hide all"));
    hideAllButton.onClick = [this]() {
        const auto node = model->getNode();
        for (int i = 0; i < node.getNumPorts(); ++i)
            node.getPort (i).setHiddenOnBlock (true);
        table.updateContent();
        table.repaint();
    };

    setSize (320, 310);

    // addAndMakeVisible (saveAsDefaultButton);
    saveAsDefaultButton.setButtonText (TRANS ("Save default"));
    saveAsDefaultButton.setEnabled (false);
}

NodePortsTable::~NodePortsTable()
{
    table.setModel (nullptr);
    model.reset();
}

void NodePortsTable::setNode (const Node& node)
{
    model->setNode (node);
    table.updateContent();
}

void NodePortsTable::refresh (int row)
{
    if (row < 0)
    {
        table.updateContent();
    }
    else
    {
        table.repaintRow (row);
    }
}

void NodePortsTable::paint (juce::Graphics& g)
{
    g.fillAll (findColour (Style::widgetBackgroundColorId));
}

void NodePortsTable::resized()
{
    auto r = getLocalBounds();

    auto r2 = r.removeFromBottom (26);
    hideAllButton.changeWidthToFitText (r2.getHeight());
    hideAllButton.setBounds (r2.removeFromRight (hideAllButton.getWidth()));
    r2.removeFromRight (3);

    showAllButton.changeWidthToFitText (r2.getHeight());
    showAllButton.setBounds (r2.removeFromRight (showAllButton.getWidth()));

    if (saveAsDefaultButton.isVisible())
    {
        saveAsDefaultButton.changeWidthToFitText (r2.getHeight());
        saveAsDefaultButton.setBounds (r2.removeFromLeft (saveAsDefaultButton.getWidth()));
    }

    r.removeFromBottom (4);
    table.setBounds (r);
}

} // namespace element