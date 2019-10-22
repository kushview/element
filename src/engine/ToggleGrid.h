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

#pragma once

namespace Element {

/** Similar to a kv::MatrixState but is intended to be used in a realtime context */
class ToggleGrid
{
public:
    /** This ctor will allocate: DO NOT create these on the stack in a realtime 
        thread, use two instances ToggleGrid::swapWith or operator= instead */
    explicit ToggleGrid (const int ins = 4, const int outs = 4)
        : numIns (ins), numOuts (outs)
    {
        jassert (ins > 0 && outs > 0);
        resize (ins, outs);
        clear();
    }

    ToggleGrid (const MatrixState& matrix)
    {
        resize (matrix.getNumRows(), matrix.getNumColumns());
        for (int i = 0; i < matrix.getNumRows(); ++i)
            for (int o = 0; o < matrix.getNumColumns(); ++o)
                toggles[i][o] = matrix.connected (i, o);
    }

    ~ToggleGrid() noexcept 
    {
        free();
    }

    inline void resize (int ins, int outs)
    {
        free();
        jassert(ins > 0 && outs > 0);
        numIns = ins;
        numOuts = outs;
        toggles = new bool* [numIns];
        for (int i = 0; i < numIns; ++i)
            toggles[i] = new bool [numOuts];
    }

    inline bool sameSizeAs (const ToggleGrid& other) const noexcept
    {
        return numIns == other.numIns && numOuts == other.numOuts;
    }

    inline bool sameSizeAs (const MatrixState& matrix) const noexcept
    {
        return numIns == matrix.getNumRows() && numOuts == matrix.getNumColumns();
    }

    inline void clear() noexcept
    {
        for (int i = 0; i < numIns; ++i)
            for (int o = 0; o < numOuts; ++o)
                toggles[i][o] = false;
    }

    inline bool get (const int in, const int out) const noexcept
    {
        jassert (isPositiveAndBelow (in, numIns) && isPositiveAndBelow (out, numOuts));
        return toggles[in][out];
    }

    inline void set (const int in, const int out, const bool value) noexcept
    {
        jassert (isPositiveAndBelow (in, numIns) && isPositiveAndBelow (out, numOuts));
        toggles[in][out] = value;
    }

    inline int getNumInputs() const noexcept    { return numIns; }
    inline int getNumOutputs() const noexcept   { return numOuts; }

    inline void swapWith (ToggleGrid& other) noexcept
    {
        std::swap (toggles, other.toggles);
    }

    ToggleGrid& operator= (const ToggleGrid& other)
    {
        if (sameSizeAs (other))
        {
            for (int i = 0; i < numIns; ++i)
                for (int o = 0; o < numOuts; ++o)
                    toggles[i][o] = other.toggles[i][o];
        }
        else
        {
            for (int i = 0; i < jmin (numIns, other.numIns); ++i)
                for (int o = 0; o < jmin (numOuts, other.numOuts); ++o)
                    toggles[i][o] = other.toggles[i][o];
        }

        return *this;
    }

private:
    int numIns, numOuts;
    bool** toggles = nullptr;

    void free()
    {
        if (toggles != nullptr)
        {
            for (int i = 0; i < numIns; ++i)
                delete [] toggles [i];
            delete [] toggles;
            toggles = nullptr;
        }
        numIns = numOuts = 0;
    }
};

}
