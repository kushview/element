// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "matrixstate.hpp"

namespace element {

/** Similar to a MatrixState but is intended to be used in a realtime context */
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
        jassert (ins > 0 && outs > 0);
        numIns = ins;
        numOuts = outs;
        toggles = new bool*[numIns];
        for (int i = 0; i < numIns; ++i)
            toggles[i] = new bool[numOuts];
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

    inline int getNumInputs() const noexcept { return numIns; }
    inline int getNumOutputs() const noexcept { return numOuts; }

    inline void swapWith (ToggleGrid& other) noexcept
    {
        std::swap (toggles, other.toggles);
        std::swap (numIns, other.numIns);
        std::swap (numOuts, other.numOuts);
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
                delete[] toggles[i];
            delete[] toggles;
            toggles = nullptr;
        }
        numIns = numOuts = 0;
    }
};

} // namespace element
