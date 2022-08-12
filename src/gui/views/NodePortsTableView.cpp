
#include "gui/views/NodePortsTableView.h"
#include "gui/ViewHelpers.h"
#include "gui/LookAndFeel.h"

namespace Element {

//==============================================================================
class NodePortsTableListBoxModel : public juce::TableListBoxModel
{
public:
    enum Columns
    {
        VisibleColumn = 1,
        NameColumn,
        TypeColumn
    };

    NodePortsTableListBoxModel (NodePortsTable& ot)
        : table (ot) {}
    ~NodePortsTableListBoxModel() {}

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
    NodePortsTable& table;
};

void NodePortsTableListBoxModel::setNode (const Node& newNode)
{
    node = newNode;
}

//==========================================================================
void NodePortsTableListBoxModel::paintRowBackground (Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    ignoreUnused (rowNumber);
    ViewHelpers::drawBasicTextRow (String(), g, width, height, rowIsSelected);
}

void NodePortsTableListBoxModel::paintCell (Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
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

void NodePortsTableListBoxModel::cellClicked (int rowNumber, int columnId, const MouseEvent&)
{
    if (columnId != VisibleColumn)
        return;
    auto port = node.getPort (rowNumber);
    port.setHiddenOnBlock (! port.isHiddenOnBlock());
    table.refresh (rowNumber);
}

//==============================================================================
NodePortsTable::NodePortsTable()
{
    setOpaque (true);

    model.reset (new NodePortsTableListBoxModel (*this));
    table.setModel (model.get());
    using NPTLBM = NodePortsTableListBoxModel;
    auto& header = table.getHeader();
    header.addColumn ("", NPTLBM::VisibleColumn, 30, 30, 30, TableHeaderComponent::defaultFlags);
    header.addColumn ("Name", NPTLBM::NameColumn, 100, 30, -1, TableHeaderComponent::defaultFlags);
    header.addColumn ("Type", NPTLBM::TypeColumn, 100, 30, -1, TableHeaderComponent::defaultFlags);
    addAndMakeVisible (table);

    addAndMakeVisible (showAllButton);
    showAllButton.setButtonText (TRANS ("Show all"));
    showAllButton.onClick = [this]() {
        const auto node = model->getNode();
        for (int i = 0; i < node.getNumPorts(); ++i)
            node.getPort(i).setHiddenOnBlock (false);
        table.updateContent();
        table.repaint();
    };

    addAndMakeVisible (hideAllButton);
    hideAllButton.setButtonText (TRANS ("Hide all"));
    hideAllButton.onClick = [this]() {
        const auto node = model->getNode();
        for (int i = 0; i < node.getNumPorts(); ++i)
            node.getPort(i).setHiddenOnBlock (true);
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

} // namespace Element