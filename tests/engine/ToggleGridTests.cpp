/*
    This file is part of Element
    Copyright (C) 2018-2019  Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

namespace Element {

class ToggleGridTest : public UnitTestBase
{
public:
    ToggleGridTest() : UnitTestBase ("Toggle Grid", "engine", "toggleGrid") { }
    virtual ~ToggleGridTest() { }

    void runTest() override
    {
        testToggleGrid();
    }

private:
    void testToggleGrid()
    {
        ToggleGrid grid1 (4, 4);
        ToggleGrid grid2 (4, 4);
        ToggleGrid grid3 (3, 4);

        beginTest ("sameSizeAs");
        expect (grid1.sameSizeAs (grid2));
        expect (! grid1.sameSizeAs (grid3));

        beginTest ("initial values");
        for (int i = 0; i < grid1.getNumInputs(); ++i)
            for (int o = 0; o < grid1.getNumOutputs(); ++o)
                expect (grid1.get (i, 0) == false);
        
        beginTest ("get/set");
        grid1.set (2, 2, true);
        expect (grid1.get (2, 2) == true);
        expect (grid2.get (2, 2) == false);

        beginTest ("swapWith");
        grid1.swapWith (grid2);
        expect (grid1.get (2, 2) == false);
        expect (grid2.get (2, 2) == true);

        beginTest ("resize");

        beginTest ("matrix state");
        MatrixState matrix (6, 6);
        matrix.set (3, 3, true);
        ToggleGrid grid4 (matrix);
        expect (grid4.getNumInputs() == matrix.getNumRows() &&
                grid4.getNumOutputs() == matrix.getNumColumns());
        expect (grid4.get (3, 3) == matrix.connected (3, 3));
    }
};

static ToggleGridTest sToggleGridTest;

}
