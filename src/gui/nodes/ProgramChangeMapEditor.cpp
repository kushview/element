#include "gui/nodes/ProgramChangeMapEditor.h"
#include "gui/ViewHelpers.h"

namespace Element {

typedef ProgramChangeMapEditor PGCME;

class PGCME::TableModel : public TableListBoxModel
{
public:
    enum ColumnId {
        Name = 1,
        InProgram,
        OutProgram
    };
    TableModel()
    {
    }

    ~TableModel()
    {

    }

    int getNumRows() override { return 10; }

    void paintRowBackground (Graphics&, int rowNumber, int width, int height,
                                        bool rowIsSelected) override
    {

    }

    void paintCell (Graphics& g, int rowNumber, int columnId,
                    int width, int height, bool rowIsSelected) override
    {
        Justification alignment = columnId == TableModel::Name 
            ? Justification::centredLeft : Justification::centred;
        int padding = columnId == TableModel::Name ? 4 : 0;
        ViewHelpers::drawBasicTextRow ("Cell", g, width, height, rowIsSelected, padding, alignment);
    }

    Component* refreshComponentForCell (int rowNumber, int columnId, bool isRowSelected,
                                        Component* existingComponentToUpdate) override
    {
        return nullptr;
    }

   #if 0
    virtual void cellClicked (int rowNumber, int columnId, const MouseEvent&);
    virtual void cellDoubleClicked (int rowNumber, int columnId, const MouseEvent&);
    virtual void backgroundClicked (const MouseEvent&);
    void sortOrderChanged (int newSortColumnId, bool isForwards) override {}
    int getColumnAutoSizeWidth (int columnId) override {}
    String getCellTooltip (int rowNumber, int columnId) override {}
    void selectedRowsChanged (int lastRowSelected) override {}
    void deleteKeyPressed (int lastRowSelected) override {}
    void returnKeyPressed (int lastRowSelected) override {}
    void listWasScrolled() override {}
    var getDragSourceDescription (const SparseSet<int>& currentlySelectedRows) override {}
   #endif
};

ProgramChangeMapEditor::ProgramChangeMapEditor()
{
    addAndMakeVisible (table);
    table.setHeaderHeight (22);
    table.setRowHeight (20);
    auto& header = table.getHeader();
    const int flags = TableHeaderComponent::visible;
    header.addColumn ("Name", TableModel::Name, 100, 100, -1, 
        flags, -1);
    header.addColumn ("Input", TableModel::InProgram, 50, 50, -1, 
        flags, -1);
    header.addColumn ("Output", TableModel::OutProgram, 50, 50, -1, 
        flags, -1);
    model.reset (new TableModel ());
    table.setModel (model.get());
    table.updateContent();

    addAndMakeVisible (addButton);
    addButton.setButtonText ("+");
    addButton.onClick = std::bind (&ProgramChangeMapEditor::addProgram, this);
    addAndMakeVisible (delButton);
    delButton.setButtonText ("-");
    delButton.onClick = std::bind (&ProgramChangeMapEditor::removeSelectedProgram, this);

    setSize (640, 360);
}

ProgramChangeMapEditor::~ProgramChangeMapEditor()
{
    table.setModel (nullptr);
    model.reset();
}
void ProgramChangeMapEditor::addProgram()
{
    DBG("add new");
}

void ProgramChangeMapEditor::removeSelectedProgram()
{
    DBG("remove selected");
    const int selected = table.getSelectedRow();
    if (! isPositiveAndBelow (selected, table.getNumRows()))
        return;
}

void ProgramChangeMapEditor::paint (Graphics& g) 
{
    
}

void ProgramChangeMapEditor::resized()
{
    auto r = getLocalBounds();
    r.removeFromBottom (2);
    auto r2 = r.removeFromBottom (18);
    r2.removeFromRight (2);
    delButton.setBounds (r2.removeFromRight (20));
    r2.removeFromRight (2);
    addButton.setBounds (r2.removeFromRight (20));

    table.setBounds (r.reduced (2));
    auto& header = table.getHeader();

    header.setColumnWidth (TableModel::Name,
        table.getWidth() - (header.getColumnWidth (TableModel::InProgram) + 
                             header.getColumnWidth (TableModel::OutProgram)));
}

}
