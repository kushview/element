// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/ui/style.hpp>

#include "nodes/midiprogrammap.hpp"
#include "nodes/midiprogrammapeditor.hpp"
#include "ui/viewhelpers.hpp"

namespace element {

typedef MidiProgramMapEditor PGCME;
typedef ReferenceCountedObjectPtr<MidiProgramMapNode> MidiProgramMapNodePtr;

class ProgramNameLabel : public Label
{
public:
    ProgramNameLabel (MidiProgramMapEditor& e)
        : editor (e)
    {
        setEditable (false, true);
    }

    ~ProgramNameLabel() {}

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

    void update() {}

protected:
    void textWasEdited() override
    {
        auto program = editor.getProgram (row);
        program.name = getText();
        editor.setProgram (row, program);
    }

private:
    MidiProgramMapEditor& editor;
    int row = -1;
};

class ProgramNumberLabel : public Label
{
public:
    ProgramNumberLabel (MidiProgramMapEditor& e, bool input)
        : editor (e), isInput (input)
    {
        setEditable (false, true);
        setJustificationType (Justification::centred);
    }

    ~ProgramNumberLabel() {}

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
    MidiProgramMapEditor& editor;
    bool isInput;
    int row = -1;
};

class PGCME::TableModel : public TableListBoxModel
{
public:
    MidiProgramMapEditor& editor;
    enum ColumnId
    {
        Name = 1,
        InProgram,
        OutProgram
    };

    TableModel (MidiProgramMapEditor& e)
        : editor (e) {}

    ~TableModel() {}

    int getNumRows() override { return editor.getNumPrograms(); }

    void paintRowBackground (Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override
    {
        ViewHelpers::drawBasicTextRow ("", g, width, height, rowIsSelected);
    }

    void paintCell (Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override
    {
        const auto program = editor.getProgram (rowNumber);
        Justification alignment = columnId == TableModel::Name
                                      ? Justification::centredLeft
                                      : Justification::centred;
        int padding = columnId == TableModel::Name ? 4 : 0;

        String text;
        switch (columnId)
        {
            case TableModel::Name:
                text = program.name;
                break;
            case TableModel::InProgram:
                text = String (1 + program.in);
                break;
            case TableModel::OutProgram:
                text = String (1 + program.out);
                break;
        }

        g.setFont (editor.getFontSize());
        ViewHelpers::drawBasicTextRow (text, g, width, height, rowIsSelected, padding, alignment);
    }

    Component* refreshComponentForCell (int rowNumber, int columnId, bool isRowSelected, Component* existing) override
    {
        const auto program = editor.getProgram (rowNumber);
        Label* label = nullptr;
        switch (columnId)
        {
            case TableModel::Name: {
                ProgramNameLabel* name = existing == nullptr ? new ProgramNameLabel (editor) : dynamic_cast<ProgramNameLabel*> (existing);
                name->setText (program.name, dontSendNotification);
                name->setRow (rowNumber);
                label = name;
            }
            break;

            case TableModel::InProgram: {
                ProgramNumberLabel* input = existing == nullptr ? new ProgramNumberLabel (editor, true) : dynamic_cast<ProgramNumberLabel*> (existing);
                input->setProgram (program.in);
                input->setRow (rowNumber);
                label = input;
            }
            break;

            case TableModel::OutProgram: {
                ProgramNumberLabel* output = existing == nullptr ? new ProgramNumberLabel (editor, false) : dynamic_cast<ProgramNumberLabel*> (existing);
                output->setProgram (program.out);
                output->setRow (rowNumber);
                label = output;
            }
            break;
        }

        if (label == nullptr)
            return nullptr;

        label->setFont (Font (FontOptions (editor.getFontSize())));

        return label;
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

MidiProgramMapEditor::MidiProgramMapEditor (const Node& node)
    : NodeEditor (node)
{
    setOpaque (true);
    addAndMakeVisible (table);
    table.setHeaderHeight (22);
    setFontSize (15.f, false);

    auto& header = table.getHeader();
    const int flags = TableHeaderComponent::visible;
    header.addColumn ("Name", TableModel::Name, 100, 100, -1, flags, -1);
    header.addColumn ("Input", TableModel::InProgram, 50, 50, -1, flags, -1);
    header.addColumn ("Output", TableModel::OutProgram, 50, 50, -1, flags, -1);
    model.reset (new TableModel (*this));
    table.setModel (model.get());
    table.updateContent();

    addAndMakeVisible (addButton);
    addButton.setButtonText ("+");
    addButton.onClick = std::bind (&MidiProgramMapEditor::addProgram, this);
    addAndMakeVisible (delButton);
    delButton.setButtonText ("-");
    delButton.onClick = std::bind (&MidiProgramMapEditor::removeSelectedProgram, this);

    addAndMakeVisible (fontSlider);
    fontSlider.setSliderStyle (Slider::LinearBar);
    fontSlider.setRange (9.0, 72.0, 1.0);
    fontSlider.setValue (fontSize, dontSendNotification);
    fontSlider.onValueChange = [this]() {
        fontSize = fontSlider.getValue();
        setFontSize (fontSize);
    };
    fontSlider.onDragEnd = [this]() {
        setFontSize (fontSize);
    };

    if (MidiProgramMapNodePtr node = getNodeObjectOfType<MidiProgramMapNode>())
    {
        setSize (node->getWidth(), node->getHeight());
        lastProgramChangeConnection = node->lastProgramChanged.connect (
            std::bind (&MidiProgramMapEditor::selectLastProgram, this));
        node->addChangeListener (this);
        node->sendChangeMessage(); // Workaround to get font size right.
        // setFontSize needs to know if this is in a plugin window
        // and doesn't know until after the ctor has returned
    }
    else
    {
        setSize (360, 540);
    }
}

MidiProgramMapEditor::~MidiProgramMapEditor()
{
    if (MidiProgramMapNodePtr node = getNodeObjectOfType<MidiProgramMapNode>())
        node->removeChangeListener (this);
    lastProgramChangeConnection.disconnect();
    addButton.onClick = nullptr;
    delButton.onClick = nullptr;
    table.setModel (nullptr);
    model.reset();
}

void MidiProgramMapEditor::changeListenerCallback (ChangeBroadcaster*)
{
    if (MidiProgramMapNodePtr node = getNodeObjectOfType<MidiProgramMapNode>())
    {
        if (isRunningInPluginWindow())
        {
            setFontSize (node->getFontSize(), false);
        }
    }

    table.updateContent();
}

void MidiProgramMapEditor::setFontControlsVisible (bool visible)
{
    fontSlider.setVisible (visible);
    resized();
}

void MidiProgramMapEditor::setFontSize (float newSize, bool updateNode)
{
    float defaultSize = getDefaultFontSize();
    fontSize = jlimit (9.f, 72.f, newSize);

    if (isRunningInPluginWindow())
    {
        table.setRowHeight (6 + static_cast<int> (1.125 * fontSize));
    }
    else
    {
        table.setRowHeight (6 + static_cast<int> (1.125 * defaultSize));
    }

    if ((double) fontSize != fontSlider.getValue())
        fontSlider.setValue (fontSize, dontSendNotification);

    table.updateContent();

    if (updateNode)
        if (MidiProgramMapNodePtr node = getNodeObjectOfType<MidiProgramMapNode>())
            node->setFontSize (fontSize);
}

bool MidiProgramMapEditor::keyPressed (const KeyPress& press)
{
    bool handled = true;

    if (press == KeyPress::rightKey)
    {
        auto row = table.getSelectedRow();
        if (isPositiveAndBelow (row, table.getNumRows()))
            this->sendProgram (row);
    }
    else
    {
        handled = false;
    }

    return handled;
}

void MidiProgramMapEditor::selectRow (int row)
{
    table.selectRow (row, false, true);
}

static bool nodeContainsProgram (const MidiProgramMapNode& node, int program)
{
    for (int j = 0; j < node.getNumProgramEntries(); ++j)
    {
        const auto entry = node.getProgramEntry (j);
        if (entry.in == program)
            return true;
    }

    return false;
}

static int nextBestProgram (const MidiProgramMapNode& node)
{
    for (int i = 0; i < 128; ++i)
    {
        if (nodeContainsProgram (node, i))
            continue;
        return i;
    }

    return -1;
}

MidiProgramMapNode::ProgramEntry MidiProgramMapEditor::getProgram (int index) const
{
    if (MidiProgramMapNodePtr node = getNodeObjectOfType<MidiProgramMapNode>())
        return node->getProgramEntry (index);
    return {};
}

void MidiProgramMapEditor::setProgram (int index, MidiProgramMapNode::ProgramEntry entry)
{
    if (MidiProgramMapNodePtr node = getNodeObjectOfType<MidiProgramMapNode>())
    {
        node->editProgramEntry (index, entry.name, entry.in, entry.out);
        table.updateContent();
    }
}

void MidiProgramMapEditor::sendProgram (int index)
{
    if (MidiProgramMapNodePtr node = getNodeObjectOfType<MidiProgramMapNode>())
    {
        const auto program = getProgram (index);
        node->sendProgramChange (program.in, 1);
    }
}

void MidiProgramMapEditor::addProgram()
{
    if (MidiProgramMapNodePtr node = getNodeObjectOfType<MidiProgramMapNode>())
    {
        const int program = nextBestProgram (*node);
        if (program >= 0)
        {
            String name = "Program ";
            name << (program + 1);
            node->addProgramEntry (name, program);
            table.updateContent();
        }
        else
        {
            DBG ("couldn't find a good program");
        }
    }
}

int MidiProgramMapEditor::getNumPrograms() const
{
    if (MidiProgramMapNodePtr node = getNodeObjectOfType<MidiProgramMapNode>())
        return node->getNumProgramEntries();
    return 0;
}

void MidiProgramMapEditor::removeSelectedProgram()
{
    if (MidiProgramMapNodePtr node = getNodeObjectOfType<MidiProgramMapNode>())
    {
        const int selected = table.getSelectedRow();
        if (! isPositiveAndBelow (selected, node->getNumProgramEntries()))
            return;
        node->removeProgramEntry (selected);
        table.updateContent();
    }
}

void MidiProgramMapEditor::paint (Graphics& g)
{
    g.fillAll (Colors::widgetBackgroundColor);
}

void MidiProgramMapEditor::resized()
{
    auto r = getLocalBounds();
    r.removeFromBottom (2);
    auto r2 = r.removeFromBottom (18);
    r2.removeFromRight (2);
    delButton.setBounds (r2.removeFromRight (20));
    r2.removeFromRight (2);
    addButton.setBounds (r2.removeFromRight (20));
    r2.removeFromRight (2);
    r2.removeFromLeft (2);
    fontSlider.setBounds (r2);

    table.setBounds (r.reduced (2));
    auto& header = table.getHeader();

    header.setColumnWidth (TableModel::Name,
                           table.getWidth() - (header.getColumnWidth (TableModel::InProgram) + header.getColumnWidth (TableModel::OutProgram)));

    if (isRunningInPluginWindow())
        if (MidiProgramMapNodePtr node = getNodeObjectOfType<MidiProgramMapNode>())
            node->setSize (getWidth(), getHeight());
}

void MidiProgramMapEditor::setStoreSize (const bool storeSize)
{
    if (storeSize == storeSizeInNode)
        return;
    storeSizeInNode = storeSize;
    if (storeSizeInNode)
        if (MidiProgramMapNodePtr node = getNodeObjectOfType<MidiProgramMapNode>())
            node->setSize (getWidth(), getHeight());
}

void MidiProgramMapEditor::selectLastProgram()
{
    MidiProgramMapNodePtr node = getNodeObjectOfType<MidiProgramMapNode>();
    if (! node)
        return;
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

} // namespace element
