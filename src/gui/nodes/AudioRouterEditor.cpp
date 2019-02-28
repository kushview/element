
#include "engine/nodes/AudioRouterNode.h"
#include "gui/nodes/AudioRouterEditor.h"
#include "gui/LookAndFeel.h"
#include "gui/Artist.h"
#include "Common.h"


namespace Element {

class AudioRouterMatrix : public kv::PatchMatrixComponent
{
public:
    AudioRouterMatrix (AudioRouterEditor& ed)
        : editor (ed)
    {
        setMatrixCellSize (48);
        setSize (getRowThickness() * 4, 
                 getColumnThickness() * 4);
        setRepaintsOnMouseActivity (true);
    }

    int getNumColumns() override    { return editor.getMatrixState().getNumColumns(); }
    int getNumRows() override       { return editor.getMatrixState().getNumRows(); }

    void paintMatrixCell (Graphics& g, const int width, const int height,
                                       const int row, const int column) override
    {
        auto& matrix = editor.getMatrixState();
        const int gridPadding = 1;
        bool useHighlighting = true;

        if (useHighlighting &&
                (mouseIsOverCell (row, column) && ! matrix.connected (row, column)))
        {
            g.setColour (LookAndFeel::elementBlue.withAlpha (0.4f));
            g.fillRect (0, 0, width - gridPadding, height - gridPadding);
        }
        else if ((mouseIsOverRow(row) || mouseIsOverColumn(column)) && !matrix.connected (row, column))
        {
            g.setColour (LookAndFeel::elementBlue.withAlpha (0.3f));
            g.fillRect (0, 0, width - gridPadding, height - gridPadding);
        }
        else
        {
            g.setColour (matrix.connected (row, column) ?
                            Colour (kv::LookAndFeel_KV1::elementBlue.brighter()) :
                            Colour (kv::LookAndFeel_KV1::defaultMatrixCellOffColor));
    
            g.fillRect (0, 0, width - gridPadding, height - gridPadding);
        }
    }

    void matrixCellClicked (const int row, const int col, const MouseEvent& ev) override
    {
        ignoreUnused (ev);
        auto& matrix = editor.getMatrixState();
        matrix.toggleCell (row, col);
        editor.applyMatrix();
        repaint();
    }

    void matrixBackgroundClicked (const MouseEvent& ev) override { }

    void matrixHoveredCellChanged (const int prevRow, const int prevCol,
                                   const int newRow,  const int newCol) override
    {
        ignoreUnused (prevRow, prevCol, newRow, newCol);
        repaint();
    }

private:
    AudioRouterEditor& editor;
};

class AudioRouterEditor::Content : public Component
{
    int padding = 10;
    int labelWidth = 60;
    Rectangle<int> matrixArea;
public:
    Content (AudioRouterEditor& o)
        : owner (o)
    {
        setOpaque (true);
        matrix.reset (new AudioRouterMatrix (o));
        addAndMakeVisible (matrix.get());
        setSize (padding + labelWidth + matrix->getWidth(), 
                 padding + labelWidth + matrix->getHeight());
        matrixArea = { labelWidth, padding, matrix->getWidth(), matrix->getHeight() };
    }

    void resized() override
    {
        auto size = jlimit (24, 44, 
            roundToInt ((double)(getWidth() - labelWidth - 32) / (double)matrix->getNumRows()));
        matrix->setMatrixCellSize (size, size);

        matrixArea = { labelWidth, padding, 
                matrix->getRowThickness() * matrix->getNumRows(), 
                matrix->getColumnThickness() * matrix->getNumColumns() };

        matrix->setBounds (matrixArea);
    }

    void paint (Graphics& g) override
    {
        g.fillAll (LookAndFeel::contentBackgroundColor);
        Rectangle<int> box (0, padding, labelWidth - padding, matrix->getHeight());
        auto rowThickness = matrix->getRowThickness();
        auto colThickness = matrix->getColumnThickness();

        g.setColour (LookAndFeel::textColor);
        for (int row = 0; row < owner.getMatrixState().getNumRows(); ++row)
            g.drawText (String("Ch. ") + String(row + 1), box.removeFromTop(rowThickness),
                Justification::centredRight, false);

        box = { matrix->getX(), matrix->getBottom() + 10, matrix->getWidth(), 50 };
        
        for (int col = 0; col < owner.getMatrixState().getNumColumns(); ++col)
        {
            auto r  = box.removeFromLeft (colThickness);
            g.setColour(LookAndFeel::textColor);
            Artist::drawVerticalText (g, String("Ch. ") + String(col + 1), r,
                                         Justification::centredRight);
        }
    }

private:
    friend class AudioRouterEditor;
    AudioRouterEditor& owner;
    std::unique_ptr<AudioRouterMatrix> matrix;
};

AudioRouterEditor::AudioRouterEditor (const Node& node)
    : NodeEditorComponent (node)
{
    setOpaque (true);
    content.reset (new Content (*this));
    addAndMakeVisible (content.get());
    if (auto* const node = getNodeObjectOfType<AudioRouterNode>())
    {
        changeListenerCallback (node); // initial gui state
        node->addChangeListener (this);
    }

    setSize (100, 100);
}

AudioRouterEditor::~AudioRouterEditor()
{
    if (auto* const node = getNodeObjectOfType<AudioRouterNode>())
        node->removeChangeListener (this);
    content.reset();
}

void AudioRouterEditor::applyMatrix()
{
    if (auto* const node = getNodeObjectOfType<AudioRouterNode>())
        node->setMatrixState (matrix);
}

void AudioRouterEditor::changeListenerCallback (ChangeBroadcaster*)
{
    if (auto* const node = getNodeObjectOfType<AudioRouterNode>())
    {
        matrix = node->getMatrixState();
        content->matrix->repaint();
    }
}

void AudioRouterEditor::resized()
{
    content->setBounds (getLocalBounds());
}

void AudioRouterEditor::paint (Graphics& g)
{
    g.fillAll (Colours::black);
}

}
