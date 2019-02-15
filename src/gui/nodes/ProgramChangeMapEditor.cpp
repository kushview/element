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
        ViewHelpers::drawBasicTextRow ("Cell", g, width, height, rowIsSelected);
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
    const int flags = TableHeaderComponent::notSortable;
    header.addColumn ("Name", TableModel::Name, 100, 100, -1, flags, -1);
    header.addColumn ("Program In", TableModel::InProgram, 100, 50, -1, flags, -1);
    header.addColumn ("Program Out", TableModel::OutProgram, 100, 50, -1, flags, -1);
    model.reset (new TableModel());
    table.setModel (model.get());
    table.updateContent();
    setSize (640, 360);
}

ProgramChangeMapEditor::~ProgramChangeMapEditor()
{
    table.setModel (nullptr);
    model.reset();
}

void ProgramChangeMapEditor::paint (Graphics& g) 
{
    
}

void ProgramChangeMapEditor::resized()
{
    table.setBounds (getLocalBounds());
}

}
