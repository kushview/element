// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce/gui_basics.hpp>

namespace element {

class QuadrantLayout : public juce::Component
{
public:
    enum Quadrant
    {
        Q1 = 0,
        Q2,
        Q3,
        Q4
    };

    QuadrantLayout();
    virtual ~QuadrantLayout();

    void paint (juce::Graphics&) override;
    virtual void resized() override;

    void setQuadrantComponent (Quadrant, juce::Component*);
    juce::Component* getQauadrantComponent (Quadrant) const;

protected:
    virtual void updateCenter();
    void setCenter (const int x, const int y);
    void setCenterX (const int x);
    void setCenterY (const int y);

private:
    juce::Rectangle<int> q1area, q2area, q3area, q4area;
    int centerX, centerY;
    juce::OwnedArray<Component> quadrants;
    bool deleteQuadrants;
    void updateQuadrantBounds();
};

class PatchMatrixComponent : public juce::Component
{
public:
    PatchMatrixComponent();
    virtual ~PatchMatrixComponent();

    void setMatrixCellSize (const int thickness);
    void setMatrixCellSize (const int horizontal, const int vertical);

    virtual int getNumColumns() = 0;
    virtual int getNumRows() = 0;
    virtual void paintMatrixCell (juce::Graphics& g, const int width, const int height, const int row, const int column) = 0;
    virtual void matrixCellClicked (const int row, const int col, const juce::MouseEvent& ev);
    virtual void matrixBackgroundClicked (const juce::MouseEvent& ev) {}
    virtual void matrixHoveredCellChanged (const int prevRow, const int prevCol, const int newRow, const int newCol) {}

    inline bool mouseIsOverRow (const int row) const { return row >= 0 && hoveredRow >= 0 && row == hoveredRow; }

    inline bool mouseIsOverColumn (const int col) const { return col >= 0 && hoveredColumn >= 0 && col == hoveredColumn; }

    inline bool mouseIsOverCell (const int row, const int col) const { return mouseIsOverRow (row) && mouseIsOverColumn (col); }

    void mouseEnter (const juce::MouseEvent& ev) override;
    void mouseMove (const juce::MouseEvent& ev) override;
    void mouseExit (const juce::MouseEvent& ev) override;

    void mouseDown (const juce::MouseEvent& ev) override;
    void paint (juce::Graphics& g) override;

    void setThickness (const int thickness)
    {
        verticalThickness = horizontalThickness = thickness;
        repaint();
    }
    int getRowThickness() const { return verticalThickness; }
    int getColumnThickness() const { return horizontalThickness; }

    int getColumnForPixel (const int x);
    int getRowForPixel (const int y);
    void setOffsetX (const int x) { offsetX = x; }
    void setOffsetY (const int y) { offsetY = y; }

private:
    int verticalThickness, horizontalThickness;
    int offsetX, offsetY, hoveredRow, lastHoveredRow, hoveredColumn, lastHoveredColumn;
    void updateHoveredCell (const int x, const int y);
};

} // namespace element
