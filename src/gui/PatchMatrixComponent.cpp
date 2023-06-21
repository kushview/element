/*
    This file is part of the Kushview Modules for JUCE
    Copyright (c) 2017-2019  Kushview, LLC.  All rights reserved.

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

#include "gui/PatchMatrixComponent.h"

#ifndef KV_DEBUG_QUADS
#define KV_DEBUG_QUADS 0
#endif

namespace element {
using namespace juce;

QuadrantLayout::QuadrantLayout()
{
    deleteQuadrants = true;
    for (int i = Q1; i <= Q4; ++i)
        quadrants.add (nullptr);
    setSize (100, 100);
}

QuadrantLayout::~QuadrantLayout()
{
    quadrants.clearQuick (deleteQuadrants);
}

void QuadrantLayout::paint (Graphics& g)
{
#if KV_DEBUG_QUADS
    /* enable this block if you want to see where the quadrants are being
       drawn at */
    g.setColour (Colours::red);
    g.fillRect (q1area);
    g.setColour (Colours::orange);
    g.fillRect (q2area);
    g.setColour (Colours::green);
    g.fillRect (q3area);
    g.setColour (Colours::yellow);
    g.fillRect (q4area);
#endif
}

void QuadrantLayout::resized()
{
    updateCenter();

    if (quadrants[Q1])
    {
        quadrants[Q1]->setBounds (q1area);
    }

    if (quadrants[Q2])
    {
        quadrants[Q2]->setBounds (q2area);
    }

    if (quadrants[Q3])
    {
        quadrants[Q3]->setBounds (q3area);
    }

    if (quadrants[Q4])
    {
        quadrants[Q4]->setBounds (q4area);
    }
}

void QuadrantLayout::setCenter (const int x, const int y)
{
    centerX = x;
    centerY = y;
    updateQuadrantBounds();
}

void QuadrantLayout::setCenterX (const int x)
{
    centerX = x;
    updateQuadrantBounds();
}

void QuadrantLayout::setCenterY (const int y)
{
    centerY = y;
    updateQuadrantBounds();
}

void QuadrantLayout::updateCenter()
{
    // setCenter (getWidth() - 300, 300);  // keep Q1 static

    // keeps q2, q3, and q4 static
    const int w = getWidth();
    const int h = getHeight();
    const int thicknessOnOtherQuads = 160;
    const int x = (thicknessOnOtherQuads <= w) ? thicknessOnOtherQuads : 0;
    const int y = (h - thicknessOnOtherQuads >= 0) ? h - thicknessOnOtherQuads : 0;

    setCenter (x, y);
}

void QuadrantLayout::updateQuadrantBounds()
{
    q1area.setBounds (centerX, 0, getWidth() - centerX, centerY);
    q2area.setBounds (0, 0, centerX, centerY);
    q3area.setBounds (0, centerY, centerX, getHeight() - centerY);
    q4area.setBounds (centerX, centerY, getWidth() - centerX, getHeight() - centerY);
}

void QuadrantLayout::setQuadrantComponent (QuadrantLayout::Quadrant q, Component* c)
{
    addAndMakeVisible (quadrants.set (q, c, deleteQuadrants));
}

Component* QuadrantLayout::getQauadrantComponent (QuadrantLayout::Quadrant q) const
{
    return quadrants.getUnchecked (q);
}

PatchMatrixComponent::PatchMatrixComponent()
{
    hoveredRow = hoveredColumn = lastHoveredRow = lastHoveredColumn = -1;
    verticalThickness = horizontalThickness = 30;
    offsetX = offsetY = 0;
}

PatchMatrixComponent::~PatchMatrixComponent()
{
}

void PatchMatrixComponent::matrixCellClicked (const int, const int, const MouseEvent&)
{
    // subclass must implement
}

void PatchMatrixComponent::updateHoveredCell (const int x, const int y)
{
    lastHoveredRow = hoveredRow;
    hoveredRow = getRowForPixel (y);
    lastHoveredColumn = hoveredColumn;
    hoveredColumn = getColumnForPixel (x);
    if (lastHoveredRow != hoveredRow || lastHoveredColumn != hoveredColumn)
        matrixHoveredCellChanged (lastHoveredRow, lastHoveredColumn, hoveredRow, hoveredColumn);
}

void PatchMatrixComponent::mouseDown (const MouseEvent& ev)
{
    const int row = getRowForPixel (ev.y);
    const int col = getColumnForPixel (ev.x);
    if (row >= 0 && col >= 0 && row < getNumRows() && col < getNumColumns())
        matrixCellClicked (row, col, ev);
    else
        matrixBackgroundClicked (ev);
}

void PatchMatrixComponent::mouseEnter (const MouseEvent& ev)
{
    updateHoveredCell (ev.x, ev.y);
    repaint();
}

void PatchMatrixComponent::mouseMove (const MouseEvent& ev)
{
    updateHoveredCell (ev.x, ev.y);
}

void PatchMatrixComponent::mouseExit (const MouseEvent& ev)
{
    hoveredRow = hoveredColumn = lastHoveredRow = lastHoveredColumn = -1;
    repaint();
}

void PatchMatrixComponent::paint (Graphics& g)
{
    if (getNumColumns() <= 0 || getNumRows() <= 0)
        return;

    const int xs = (offsetX % horizontalThickness);
    const int ys = (offsetY % verticalThickness);
    const int cs = getColumnForPixel (0);
    const int w = horizontalThickness;
    const int h = verticalThickness;

    int row = getRowForPixel (0);
    for (int y = ys; y < getHeight(); y += h)
    {
        int col = cs;
        for (int x = xs; x < getWidth(); x += w)
        {
            g.saveState();
            g.setOrigin (x, y);
            paintMatrixCell (g, w, h, row, col);
            g.restoreState();
            if (++col == getNumColumns())
                break;
        }
        if (++row == getNumRows())
            break;
    }
}

int PatchMatrixComponent::getColumnForPixel (const int x)
{
    return (x - offsetX) / horizontalThickness;
}

int PatchMatrixComponent::getRowForPixel (const int y)
{
    return (y - offsetY) / verticalThickness;
}

void PatchMatrixComponent::setMatrixCellSize (const int thickness)
{
    setMatrixCellSize (thickness, thickness);
}

void PatchMatrixComponent::setMatrixCellSize (const int horizontal, const int vertical)
{
    horizontalThickness = horizontal;
    verticalThickness = vertical;
    jassert (horizontalThickness > 0 && verticalThickness > 0);
    resized();
}

} // namespace element
