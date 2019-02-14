#include "gui/nodes/ProgramChangeMapEditor.h"

namespace Element {

typedef ProgramChangeMapEditor PGCME;

class PGCME::TableModel : public TableListBoxModel
{
public:
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
        g.setColour (Colours::white);
        g.drawText ("Cell", 0, 0, width, height, Justification::centredLeft);
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
    auto& header = table.getHeader();
    header.addColumn ("In Program", 1, 100);
    header.addColumn ("Out Program", 2, 100);
    setSize (640, 360);
}

ProgramChangeMapEditor::~ProgramChangeMapEditor()
{

}

void ProgramChangeMapEditor::paint(Graphics& g) {}

void ProgramChangeMapEditor::resized()
{
    table.setBounds (getLocalBounds());
}

}
