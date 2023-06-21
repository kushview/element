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
