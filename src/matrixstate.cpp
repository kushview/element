// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "matrixstate.hpp"

namespace element {
using namespace juce;

void MatrixState::setFrom (const MatrixState& o)
{
    for (int row = jmin (getNumRows(), o.getNumRows()); --row >= 0;)
        for (int col = jmin (getNumColumns(), o.getNumColumns()); --col >= 0;)
            set (row, col, o.connected (row, col));
}

void MatrixState::resize (int r, int c, bool retain)
{
    if (r < 0)
        r = 0;
    if (c < 0)
        c = 0;

    BigInteger bits;
    bits.setRange (0, r * c, false);

    // TODO: retain set bits
    if (retain)
    {
        const auto old = *this;
        numRows = r;
        numColumns = c;
        toggled.swapWith (bits);
        setFrom (old);
    }
    else
    {
        numRows = r;
        numColumns = c;
        toggled.swapWith (bits);
    }
}

} // namespace element
