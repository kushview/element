/*
    This file is part of the Kushview Modules for JUCE
    Copyright (c) 2016-2019  Kushview, LLC.  All rights reserved.

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

#pragma once

#include "JuceHeader.h"

namespace element {

class MatrixState
{
public:
    MatrixState()
    {
        numRows = numColumns = 0;
    }

    MatrixState (const int rows, const int cols)
    {
        numRows = rows;
        numColumns = cols;
        jassert (rows >= 0 && cols >= 0);
        toggled.setRange (0, numRows * numColumns, false);
    }

    virtual ~MatrixState() {}

    inline const bool isEmpty() const { return numRows <= 0 && numColumns <= 0; }
    inline const bool isNotEmpty() const { return ! isEmpty(); }
    inline const bool isValid (int row, int column)
    {
        return juce::isPositiveAndBelow (row, numRows) || juce::isPositiveAndBelow (column, numColumns);
    }

    inline int getIndexForCell (int row, int column) const { return (row * numColumns) + column; }
    inline int getNumRows() const { return numRows; }
    inline int getNumColumns() const { return numColumns; }

    inline void connect (int row, int column)
    {
        if (isValid (row, column))
        {
            const int index = getIndexForCell (row, column);
            toggled.setBit (index, true);
        }
    }

    inline void set (int r, int c, bool on)
    {
        if (isValid (r, c))
        {
            const int index = getIndexForCell (r, c);
            toggled.setBit (index, on);
        }
    }

    void setFrom (const MatrixState&);

    inline void disconnect (int row, int column)
    {
        if (! isValid (row, column))
            return;
        const int index = getIndexForCell (row, column);
        toggled.setBit (index, false);
    }

    inline bool connected (const int row, const int col) const
    {
        return toggled[getIndexForCell (row, col)];
    }

    inline bool isCellToggled (int r, int c) const { return connected (r, c); }
    inline bool toggleCell (int row, int column)
    {
        if (isValid (row, column))
        {
            const int index = getIndexForCell (row, column);
            toggled.setBit (index, ! toggled[index]);
            return true;
        }

        return false;
    }

    inline bool connectedAtIndex (const int index) const { return toggled[index]; }

#if JUCE_MODULE_AVAILABLE_juce_data_structures
    inline juce::ValueTree createValueTree (const juce::String& type = "matrix") const
    {
        juce::ValueTree tree (juce::Identifier::isValidIdentifier (type) ? type : "matrix");
        tree.setProperty ("numRows", numRows, nullptr);
        tree.setProperty ("numColumns", numColumns, nullptr);
        tree.setProperty ("toggled", toggled.toString (2), nullptr);
        return tree;
    }

    inline void restoreFromValueTree (const juce::ValueTree& tree)
    {
        numRows = tree.getProperty ("numRows", 0);
        numColumns = tree.getProperty ("numColumns", 0);
        toggled.parseString (tree.getProperty ("toggled").toString(), 2);
    }
#endif

    /** Resize the matrix to the specified rows and column sizes */
    void resize (int newNumRows, int newNumColumns, bool retain = false);

    /** Returns true if the matrices are the same size */
    inline bool sameSizeAs (const MatrixState& o) const
    {
        return this->numRows == o.numRows && this->numColumns == o.numColumns;
    }

    MatrixState (const MatrixState& o)
    {
        this->operator= (o);
    }

    MatrixState& operator= (const MatrixState& o)
    {
        this->numRows = o.numRows;
        this->numColumns = o.numColumns;
        this->toggled = o.toggled;
        return *this;
    }

    const bool operator== (const MatrixState& o) const
    {
        return this->sameSizeAs (o) && this->toggled == o.toggled;
    }

private:
    juce::BigInteger toggled;
    int numRows, numColumns;
};

} // namespace element
