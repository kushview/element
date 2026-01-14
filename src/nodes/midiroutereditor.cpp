// SPDX-License-Identifier: GPL-3.0-or-later
/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <element/ui/style.hpp>

#include "nodes/midirouter.hpp"
#include "nodes/midiroutereditor.hpp"
#include "ui/patchmatrix.hpp"

#include "common.hpp"

namespace element {

class MidiRouterMatrix : public PatchMatrixComponent
{
public:
    MidiRouterMatrix (MidiRouterEditor& ed)
        : editor (ed)
    {
        setMatrixCellSize (48);
        setSize (getRowThickness() * 4,
                 getColumnThickness() * 4);
        setRepaintsOnMouseActivity (true);
    }

    int getNumColumns() override { return editor.getMatrixState().getNumColumns(); }
    int getNumRows() override { return editor.getMatrixState().getNumRows(); }

    void paintMatrixCell (Graphics& g, const int width, const int height, const int row, const int column) override
    {
        auto& matrix = editor.getMatrixState();
        const int gridPadding = 1;
        bool useHighlighting = true;

        if (useHighlighting && (mouseIsOverCell (row, column) && ! matrix.connected (row, column)))
        {
            g.setColour (Colors::elemental.withAlpha (0.4f));
            g.fillRect (0, 0, width - gridPadding, height - gridPadding);
        }
        else if ((mouseIsOverRow (row) || mouseIsOverColumn (column)) && ! matrix.connected (row, column))
        {
            g.setColour (Colors::elemental.withAlpha (0.3f));
            g.fillRect (0, 0, width - gridPadding, height - gridPadding);
        }
        else
        {
            g.setColour (matrix.connected (row, column) ? Colour (Colors::elemental.brighter()) : Colour (LookAndFeel_E1::defaultMatrixCellOffColor));

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

    void matrixBackgroundClicked (const MouseEvent& ev) override {}

    void matrixHoveredCellChanged (const int prevRow, const int prevCol, const int newRow, const int newCol) override
    {
        ignoreUnused (prevRow, prevCol, newRow, newCol);
        repaint();
    }

private:
    MidiRouterEditor& editor;
};

class MidiRouterEditor::Content : public Component
{
public:
    Content (MidiRouterEditor& o)
        : owner (o)
    {
        setOpaque (true);
        matrix.reset (new MidiRouterMatrix (o));
        addAndMakeVisible (matrix.get());

        // addAndMakeVisible (slider);
        slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (Slider::NoTextBox, true, 1, 1);
        slider.setRange (0.001, 2.0);

        slider.onValueChange = [] {};

        setSize (padding + labelWidth + matrix->getWidth(),
                 padding + labelWidth + matrix->getHeight());
        matrixArea = { labelWidth, padding, matrix->getWidth(), matrix->getHeight() };
    }

    ~Content()
    {
        slider.onValueChange = nullptr;
    }

    void resized() override
    {
        auto size = jlimit (24, 36, roundToInt ((double) (getWidth() - labelWidth - 32) / (double) matrix->getNumRows()));
        matrix->setMatrixCellSize (size, size);

        matrixArea = { labelWidth, padding, matrix->getRowThickness() * matrix->getNumRows(), matrix->getColumnThickness() * matrix->getNumColumns() };

        matrix->setBounds (matrixArea);
        if (slider.isVisible())
            slider.setBounds (matrixArea.getX() - size + 2, matrixArea.getBottom() + 4, size - 2, size - 2);
    }

    void paint (Graphics& g) override
    {
        g.fillAll (Colors::contentBackgroundColor);
        Rectangle<int> box (0, padding, labelWidth - padding, matrix->getHeight());
        auto rowThickness = matrix->getRowThickness();
        auto colThickness = matrix->getColumnThickness();

        g.setColour (Colors::textColor);
        for (int row = 0; row < owner.getMatrixState().getNumRows(); ++row)
            g.drawText (String ("Ch. ") + String (row + 1), box.removeFromTop (rowThickness), Justification::centredRight, false);

        box = { matrix->getX(), matrix->getBottom() + 10, matrix->getWidth(), 50 };

        for (int col = 0; col < owner.getMatrixState().getNumColumns(); ++col)
        {
            auto r = box.removeFromLeft (colThickness);
            g.setColour (Colors::textColor);
            Style::drawVerticalText (g, String ("Ch. ") + String (col + 1), r, Justification::centredRight);
        }
    }

private:
    friend class MidiRouterEditor;
    int padding = 10;
    int labelWidth = 60;
    Rectangle<int> matrixArea;
    MidiRouterEditor& owner;
    Slider slider;
    std::unique_ptr<MidiRouterMatrix> matrix;
};

MidiRouterEditor::MidiRouterEditor (const Node& node)
    : NodeEditor (node)
{
    setOpaque (true);
    content.reset (new Content (*this));
    addAndMakeVisible (content.get());
    if (auto* const node = getNodeObjectOfType<MidiRouterNode>())
    {
        changeListenerCallback (node); // initial gui state
        node->addChangeListener (this);
    }

    setSize (content->getWidth(), content->getHeight());
}

MidiRouterEditor::~MidiRouterEditor()
{
    if (auto* const node = getNodeObjectOfType<MidiRouterNode>())
        node->removeChangeListener (this);
    content.reset();
}

void MidiRouterEditor::applyMatrix()
{
    if (auto* const node = getNodeObjectOfType<MidiRouterNode>())
        node->setMatrixState (matrix);
}

void MidiRouterEditor::changeListenerCallback (ChangeBroadcaster*)
{
    if (auto* const node = getNodeObjectOfType<MidiRouterNode>())
    {
        matrix = node->getMatrixState();
        content->resized();
        content->matrix->repaint();
    }
}

void MidiRouterEditor::resized()
{
    content->setBounds (getLocalBounds());
}

void MidiRouterEditor::paint (Graphics& g)
{
    g.fillAll (Colours::black);
}

} // namespace element
