#include "engine/nodes/ProgramChangeMapNode.h"
#include "gui/nodes/ProgramChangeMapEditor.h"
#include "gui/ViewHelpers.h"

namespace Element {

typedef ProgramChangeMapEditor PGCME;
typedef ReferenceCountedObjectPtr<ProgramChangeMapNode> ProgramChangeMapNodePtr;

class ProgramNameLabel : public Label
{
public:
    ProgramNameLabel (ProgramChangeMapEditor& e)
        : editor (e)
    {
        setEditable (false, true);
    }

    ~ProgramNameLabel() { }

    void setRow (int r) { row = r; }

    void mouseDown (const MouseEvent& ev) override
    {
        if (ev.getNumberOfClicks() == 1)
        {
            editor.selectRow (row);
            editor.sendProgram (row);
        }
        else
        {
            Label::mouseDown (ev);
        }
    }

    void update() { }

protected:
    void textWasEdited() override
    {
        auto program = editor.getProgram (row);
        program.name = getText();
        editor.setProgram (row, program);
    }

private:
    ProgramChangeMapEditor& editor;
    int row = -1;
};

class ProgramNumberLabel : public Label
{
public:
    ProgramNumberLabel (ProgramChangeMapEditor& e, bool input)
        : editor(e), isInput (input)
    { 
        setEditable (false, true);
        setJustificationType (Justification::centred);
    }
    
    ~ProgramNumberLabel() { }

    void mouseDown (const MouseEvent& ev) override
    {
        if (ev.getNumberOfClicks() == 1)
        {
            editor.selectRow (row);
            editor.sendProgram (row);
        }
        else
        {
            Label::mouseDown (ev);
        }
    }

    void setRow (int r) { row = r; }

    void setProgram (int program)
    {
        setText (String (jlimit (0, 127, program) + 1), dontSendNotification);
    }

protected:
    void editorShown (TextEditor* textEditor) override
    {
        textEditor->setInputRestrictions (3, "0123456789");
    }

    void textWasEdited() override
    {
        const int newProgram = jlimit (1, 128, getText().getIntValue()) - 1;
        auto program = editor.getProgram (row);
        if (isInput)
            program.in = newProgram;
        else
            program.out = newProgram;
        editor.setProgram (row, program);
    }

private:
    ProgramChangeMapEditor& editor;
    bool isInput;
    int row = -1;
};

class PGCME::TableModel : public TableListBoxModel
{
public:
    ProgramChangeMapEditor& editor;
    enum ColumnId
    {
        Name = 1,
        InProgram,
        OutProgram
    };

    TableModel (ProgramChangeMapEditor& e) : editor (e) { }
    ~TableModel() { }

    int getNumRows() override { return editor.getNumPrograms(); }

    void paintRowBackground (Graphics& g, int rowNumber, int width, int height,
                                        bool rowIsSelected) override
    {
        ViewHelpers::drawBasicTextRow ("", g, width, height, rowIsSelected);
    }

    void paintCell (Graphics& g, int rowNumber, int columnId,
                    int width, int height, bool rowIsSelected) override
    {
        const auto program = editor.getProgram (rowNumber);
        Justification alignment = columnId == TableModel::Name 
            ? Justification::centredLeft : Justification::centred;
        int padding = columnId == TableModel::Name ? 4 : 0;

        String text;
        switch (columnId)
        {
            case TableModel::Name:       text = program.name; break;
            case TableModel::InProgram:  text = String (1 + program.in); break;
            case TableModel::OutProgram: text = String (1 + program.out); break;
        }

        ViewHelpers::drawBasicTextRow (text, g, width, height, rowIsSelected, padding, alignment);
    }

    Component* refreshComponentForCell (int rowNumber, int columnId, bool isRowSelected,
                                        Component* existing) override
    {
        const auto program = editor.getProgram (rowNumber);
        switch (columnId)
        {
            case TableModel::Name:
            {
                ProgramNameLabel* name = existing == nullptr ? new ProgramNameLabel (editor) :
                    dynamic_cast<ProgramNameLabel*> (existing);
                name->setText (program.name, dontSendNotification);
                name->setRow (rowNumber);
                return name;
            } break;

            case TableModel::InProgram:
            {
                ProgramNumberLabel* input = existing == nullptr ? new ProgramNumberLabel (editor, true) :
                    dynamic_cast<ProgramNumberLabel*> (existing);
                input->setProgram (program.in);
                input->setRow (rowNumber);
                return input;
            } break;

            case TableModel::OutProgram: 
            {
                ProgramNumberLabel* output = existing == nullptr ? new ProgramNumberLabel (editor, false) :
                    dynamic_cast<ProgramNumberLabel*> (existing);
                output->setProgram (program.out);
                output->setRow (rowNumber);
                return output;
            } break;
        }
        jassertfalse;
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

ProgramChangeMapEditor::ProgramChangeMapEditor (const Node& node)
    : NodeEditorComponent (node)
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
    model.reset (new TableModel (*this));
    table.setModel (model.get());
    table.updateContent();

    addAndMakeVisible (addButton);
    addButton.setButtonText ("+");
    addButton.onClick = std::bind (&ProgramChangeMapEditor::addProgram, this);
    addAndMakeVisible (delButton);
    delButton.setButtonText ("-");
    delButton.onClick = std::bind (&ProgramChangeMapEditor::removeSelectedProgram, this);

    if (ProgramChangeMapNodePtr node = getNodeObjectOfType<ProgramChangeMapNode>())
    {
        setSize (node->getWidth(), node->getHeight());
        lastProgramChangeConnection = node->lastProgramChanged.connect (
            std::bind (&ProgramChangeMapEditor::selectLastProgram, this));
        node->addChangeListener (this);
    }
    else
    {
        setSize (360, 540);
    }
}

ProgramChangeMapEditor::~ProgramChangeMapEditor()
{
    if (ProgramChangeMapNodePtr node = getNodeObjectOfType<ProgramChangeMapNode>())
        node->removeChangeListener (this);
    lastProgramChangeConnection.disconnect();
    addButton.onClick = nullptr;
    delButton.onClick = nullptr;
    table.setModel (nullptr);
    model.reset();
}

void ProgramChangeMapEditor::selectRow (int row)
{
    table.selectRow (row, false, true);
}

static bool nodeContainsProgram (const ProgramChangeMapNode& node, int program)
{
    for (int j = 0; j < node.getNumProgramEntries(); ++j)
    {
        const auto entry = node.getProgramEntry (j);
        if (entry.in == program)
            return true;
    }

    return false;
}

static int nextBestProgram (const ProgramChangeMapNode& node)
{
    for (int i = 0; i < 128; ++i)
    {
        if (nodeContainsProgram (node, i))
            continue;
        return i;
    }

    return -1;
}

ProgramChangeMapNode::ProgramEntry ProgramChangeMapEditor::getProgram (int index) const
{
    if (ProgramChangeMapNodePtr node = getNodeObjectOfType<ProgramChangeMapNode>())
        return node->getProgramEntry (index);
    return {};
}

void ProgramChangeMapEditor::setProgram (int index, ProgramChangeMapNode::ProgramEntry entry)
{
    if (ProgramChangeMapNodePtr node = getNodeObjectOfType<ProgramChangeMapNode>())
    {
        node->editProgramEntry (index, entry.name, entry.in, entry.out);
        table.updateContent();
    }
}

void ProgramChangeMapEditor::sendProgram (int index)
{
    if (ProgramChangeMapNodePtr node = getNodeObjectOfType<ProgramChangeMapNode>())
    {
        const auto program = getProgram (index);
        node->sendProgramChange (program.in, 1);
    }
}

void ProgramChangeMapEditor::addProgram()
{
    if (ProgramChangeMapNodePtr node = getNodeObjectOfType<ProgramChangeMapNode>())
    {
        const int program = nextBestProgram (*node);
        if (program >= 0)
        {
            String name = "Program "; name << (program + 1);
            node->addProgramEntry (name, program);
            table.updateContent();
        }
        else
        {
            DBG("couldn't find a good program");
        }
    }
}

int ProgramChangeMapEditor::getNumPrograms() const
{
    if (ProgramChangeMapNodePtr node = getNodeObjectOfType<ProgramChangeMapNode>())
        return node->getNumProgramEntries();
    return 0;
}

void ProgramChangeMapEditor::removeSelectedProgram()
{
    if (ProgramChangeMapNodePtr node = getNodeObjectOfType<ProgramChangeMapNode>())
    {    
        const int selected = table.getSelectedRow();
        if (! isPositiveAndBelow (selected, node->getNumProgramEntries()))
            return;
        node->removeProgramEntry (selected);
        table.updateContent();
    }
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

    if (storeSizeInNode)
        if (ProgramChangeMapNodePtr node = getNodeObjectOfType<ProgramChangeMapNode>())
            node->setSize (getWidth(), getHeight());
}

void ProgramChangeMapEditor::setStoreSize (const bool storeSize)
{
    if (storeSize == storeSizeInNode)
        return;
    storeSizeInNode = storeSize;
    if (storeSizeInNode)
        if (ProgramChangeMapNodePtr node = getNodeObjectOfType<ProgramChangeMapNode>())
            node->setSize (getWidth(), getHeight());
}

void ProgramChangeMapEditor::selectLastProgram()
{
    ProgramChangeMapNodePtr node = getNodeObjectOfType<ProgramChangeMapNode>();
    if (! node) return;
    const auto lastProgram = node->getLastProgram();
    for (int i = 0; i < getNumPrograms(); ++i)
    {
        const auto program = getProgram (i);
        if (program.in == lastProgram)
        {
            selectRow (i);
            break;
        }
    }
}

}
