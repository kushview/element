
#include "gui/views/NodePortsTableView.h"
#include "gui/ViewHelpers.h"

namespace Element {

//==============================================================================
NodePortsTable::NodePortsTable()
{
    setModel (this);
    auto& header = getHeader();
    header.addColumn ("", VisibleColumn, 30, 30, 30, TableHeaderComponent::defaultFlags);
    header.addColumn ("Name", NameColumn, 100, 30, -1, TableHeaderComponent::defaultFlags);
    header.addColumn ("Type", TypeColumn, 100, 30, -1, TableHeaderComponent::defaultFlags);
    setSize (320, 280);
}

NodePortsTable::~NodePortsTable()
{
    setModel (nullptr);
}

void NodePortsTable::setNode (const Node& newNode)
{
    node = newNode;
    updateContent();
}

//==========================================================================
void NodePortsTable::paintRowBackground (Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    ignoreUnused (rowNumber);
    ViewHelpers::drawBasicTextRow (String(), g, width, height, rowIsSelected);
}

void NodePortsTable::paintCell (Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
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
            getLookAndFeel().drawTickBox (
                g, *this, r.getX(), r.getY(), r.getWidth(), r.getHeight(), ! port.isHiddenOnBlock(), true, false, false);
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

void NodePortsTable::cellClicked (int rowNumber, int columnId, const MouseEvent&)
{
    if (columnId != VisibleColumn)
        return;
    auto port = node.getPort (rowNumber);
    port.setHiddenOnBlock (! port.isHiddenOnBlock());
    repaintRow (rowNumber);
}

//==============================================================================
class NodePortsTableView::Content : public Component
{
public:
};

//==============================================================================
NodePortsTableView::NodePortsTableView()
{
    setName ("NodePortsTableView");
    content.reset (new Content());
    addAndMakeVisible (content.get());
}

NodePortsTableView::~NodePortsTableView()
{
    content.reset();
}

} // namespace Element