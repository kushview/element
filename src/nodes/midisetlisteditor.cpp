// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/ui/style.hpp>

#include "nodes/midisetlist.hpp"
#include "nodes/midisetlisteditor.hpp"
#include "ui/viewhelpers.hpp"

namespace element {

typedef MidiSetListEditor MSLE;
typedef ReferenceCountedObjectPtr<MidiSetListProcessor> MidiSetListProcessorPtr;

class MidiSetListProgramNameLabel : public Label
{
public:
    MidiSetListProgramNameLabel (MidiSetListEditor& e)
        : editor (e)
    {
        setEditable (false, true);
    }

    ~MidiSetListProgramNameLabel() {}

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
        std::clog << "text was edited\n";
        auto program = editor.getProgram (row);
        program.name = getText();
        editor.setProgram (row, program);
    }

private:
    MidiSetListEditor& editor;
    int row = -1;
};

class MidiSetListProgramNumberLabel : public Label
{
public:
    MidiSetListProgramNumberLabel (MidiSetListEditor& e, bool input)
        : editor (e), isInput (input)
    {
        setEditable (false, true);
        setJustificationType (Justification::centred);
    }

    ~MidiSetListProgramNumberLabel() {}

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
    MidiSetListEditor& editor;
    bool isInput;
    int row = -1;
};

class MidiSetListProgramTempoLabel : public Label
{
public:
    MidiSetListProgramTempoLabel (MidiSetListEditor& e)
        : editor (e)
    {
        setEditable (false, true);
        setJustificationType (Justification::centred);
    }

    ~MidiSetListProgramTempoLabel() {}

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

    void setTempo (double tempo)
    {
        if (tempo > 0.0)
            tempo = jlimit (20.0, 999.0, tempo);
        if (tempo >= 20.0 && tempo <= 999.0)
            setText (String (tempo, 2) + " bpm", dontSendNotification);
        else
            setText ("N/A", dontSendNotification);
    }

protected:
    void editorShown (TextEditor* textEditor) override
    {
        textEditor->setInputRestrictions (0, ".0123456789");
    }

    void textWasEdited() override
    {
        if (row < 0)
            return;

        const double newTempo = getText().getDoubleValue();
        auto program = editor.getProgram (row);
        program.tempo = newTempo;
        editor.setProgram (row, program);
    }

private:
    MidiSetListEditor& editor;
    int row = -1;
};

class MSLE::TableModel : public TableListBoxModel
{
public:
    MidiSetListEditor& editor;
    enum ColumnId
    {
        InProgram = 1,
        Name,
        Tempo,
        OutProgram
    };

    TableModel (MidiSetListEditor& e)
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
            case TableModel::Tempo:
                text = String (120.00, 2) + " bpm";
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
                auto* name = existing == nullptr
                                 ? new MidiSetListProgramNameLabel (editor)
                                 : dynamic_cast<MidiSetListProgramNameLabel*> (existing);
                name->setText (program.name, dontSendNotification);
                name->setRow (rowNumber);
                label = name;
            }
            break;

            case TableModel::InProgram: {
                auto* input = existing == nullptr
                                  ? new MidiSetListProgramNumberLabel (editor, true)
                                  : dynamic_cast<MidiSetListProgramNumberLabel*> (existing);
                input->setProgram (program.in);
                input->setRow (rowNumber);
                label = input;
                break;
            }

            case TableModel::OutProgram: {
                auto* output = existing == nullptr
                                   ? new MidiSetListProgramNumberLabel (editor, false)
                                   : dynamic_cast<MidiSetListProgramNumberLabel*> (existing);
                output->setProgram (program.out);
                output->setRow (rowNumber);
                label = output;
                break;
            }

            case TableModel::Tempo: {
                auto* t = existing == nullptr
                              ? new MidiSetListProgramTempoLabel (editor)
                              : dynamic_cast<MidiSetListProgramTempoLabel*> (existing);
                t->setTempo (program.tempo);
                t->setRow (rowNumber);
                label = t;
                break;
            }
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

MidiSetListEditor::MidiSetListEditor (const Node& node)
    : NodeEditor (node)
{
    setOpaque (true);
    addAndMakeVisible (table);
    table.setHeaderHeight (22);
    setFontSize (15.f, false);

    auto& header = table.getHeader();
    const int flags = TableHeaderComponent::visible;
    header.addColumn ("IN", TableModel::InProgram, 50, 50, -1, flags, -1);
    header.addColumn ("NAME", TableModel::Name, 100, 100, -1, flags, -1);
    header.addColumn ("TEMPO", TableModel::Tempo, 70, 70, -1, flags, -1);
    header.addColumn ("OUT", TableModel::OutProgram, 50, 50, -1, flags, -1);
    model.reset (new TableModel (*this));
    table.setModel (model.get());
    table.updateContent();

    addAndMakeVisible (addButton);
    addButton.setButtonText ("+");
    addButton.onClick = std::bind (&MidiSetListEditor::addProgram, this);
    addAndMakeVisible (delButton);
    delButton.setButtonText ("-");
    delButton.onClick = std::bind (&MidiSetListEditor::removeSelectedProgram, this);

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

    if (MidiSetListProcessorPtr node = getNodeObjectOfType<MidiSetListProcessor>())
    {
        setSize (node->getWidth(), node->getHeight());
        lastProgramChangeConnection = node->lastProgramChanged.connect (
            std::bind (&MidiSetListEditor::selectLastProgram, this));
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

MidiSetListEditor::~MidiSetListEditor()
{
    if (MidiSetListProcessorPtr node = getNodeObjectOfType<MidiSetListProcessor>())
        node->removeChangeListener (this);
    lastProgramChangeConnection.disconnect();
    addButton.onClick = nullptr;
    delButton.onClick = nullptr;
    table.setModel (nullptr);
    model.reset();
}

void MidiSetListEditor::changeListenerCallback (ChangeBroadcaster*)
{
    if (MidiSetListProcessorPtr node = getNodeObjectOfType<MidiSetListProcessor>())
    {
        if (isRunningInPluginWindow())
        {
            setFontSize (node->getFontSize(), false);
        }
    }

    table.updateContent();
}

void MidiSetListEditor::setFontControlsVisible (bool visible)
{
    fontSlider.setVisible (visible);
    resized();
}

void MidiSetListEditor::setFontSize (float newSize, bool updateNode)
{
    fontSize = jlimit (9.f, 72.f, newSize);

    table.setRowHeight (6 + static_cast<int> (1.125 * fontSize));

    if ((double) fontSize != fontSlider.getValue())
        fontSlider.setValue (fontSize, dontSendNotification);

    updateTableHeaderSizes();
    table.updateContent();

    if (updateNode)
        if (MidiSetListProcessorPtr node = getNodeObjectOfType<MidiSetListProcessor>())
            node->setFontSize (fontSize);
}

bool MidiSetListEditor::keyPressed (const KeyPress& press)
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

void MidiSetListEditor::selectRow (int row)
{
    table.selectRow (row, false, true);
}

static bool nodeContainsProgram (const MidiSetListProcessor& node, int program)
{
    for (int j = 0; j < node.getNumProgramEntries(); ++j)
    {
        const auto entry = node.getProgramEntry (j);
        if (entry.in == program)
            return true;
    }

    return false;
}

static int nextBestProgram (const MidiSetListProcessor& node)
{
    for (int i = 0; i < 128; ++i)
    {
        if (nodeContainsProgram (node, i))
            continue;
        return i;
    }

    return -1;
}

MidiSetListProcessor::ProgramEntry MidiSetListEditor::getProgram (int index) const
{
    if (MidiSetListProcessorPtr node = getNodeObjectOfType<MidiSetListProcessor>())
        return node->getProgramEntry (index);
    return {};
}

void MidiSetListEditor::setProgram (int index, MidiSetListProcessor::ProgramEntry entry)
{
    if (MidiSetListProcessorPtr node = getNodeObjectOfType<MidiSetListProcessor>())
    {
        node->editProgramEntry (index,
                                entry.name,
                                entry.in,
                                entry.out,
                                entry.tempo);
        table.updateContent();
    }
}

void MidiSetListEditor::sendProgram (int index)
{
    if (MidiSetListProcessorPtr node = getNodeObjectOfType<MidiSetListProcessor>())
    {
        const auto program = getProgram (index);
        node->sendProgramChange (program.in, 1);
    }
}

void MidiSetListEditor::addProgram()
{
    if (MidiSetListProcessorPtr node = getNodeObjectOfType<MidiSetListProcessor>())
    {
        const int program = nextBestProgram (*node);
        if (program >= 0)
        {
            String name = "Song ";
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

int MidiSetListEditor::getNumPrograms() const
{
    if (MidiSetListProcessorPtr node = getNodeObjectOfType<MidiSetListProcessor>())
        return node->getNumProgramEntries();
    return 0;
}

void MidiSetListEditor::removeSelectedProgram()
{
    if (MidiSetListProcessorPtr node = getNodeObjectOfType<MidiSetListProcessor>())
    {
        const int selected = table.getSelectedRow();
        if (! isPositiveAndBelow (selected, node->getNumProgramEntries()))
            return;
        node->removeProgramEntry (selected);
        table.updateContent();
    }
}

void MidiSetListEditor::paint (Graphics& g)
{
    g.fillAll (Colors::widgetBackgroundColor);
}

void MidiSetListEditor::resized()
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
    updateTableHeaderSizes();

    if (isRunningInPluginWindow())
        if (MidiSetListProcessorPtr node = getNodeObjectOfType<MidiSetListProcessor>())
            node->setSize (getWidth(), getHeight());
}

void MidiSetListEditor::setStoreSize (const bool storeSize)
{
    if (storeSize == storeSizeInNode)
        return;

    storeSizeInNode = storeSize;

    if (storeSizeInNode)
        if (MidiSetListProcessorPtr node = getNodeObjectOfType<MidiSetListProcessor>())
            node->setSize (getWidth(), getHeight());
}

void MidiSetListEditor::updateTableHeaderSizes()
{
    auto& header = table.getHeader();
    GlyphArrangement glyphs;
    glyphs.addLineOfText (Font (FontOptions (getFontSize())), "120.00 bpm", 0, 0);
    const auto tempoSize = (int) glyphs.getBoundingBox (0, -1, true).getWidth() + 4;
    header.setColumnWidth (TableModel::Tempo, tempoSize);

    // clang-format on
    const auto fixedTotalSize = header.getColumnWidth (TableModel::InProgram)
                                + header.getColumnWidth (TableModel::OutProgram)
                                + header.getColumnWidth (TableModel::Tempo);

    header.setColumnWidth (TableModel::Name, table.getWidth() - fixedTotalSize);
    // clang-format off
}

void MidiSetListEditor::selectLastProgram()
{
    MidiSetListProcessorPtr node = getNodeObjectOfType<MidiSetListProcessor>();
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
