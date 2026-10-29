// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later


#include <boost/test/unit_test.hpp>
#include "engine/togglegrid.hpp"

using namespace element;
using namespace juce;

class ToggleGridUnit {
public:
    void runTest()
    {
        testToggleGrid();
    }

private:
    void testToggleGrid()
    {
        ToggleGrid grid1 (4, 4);
        ToggleGrid grid2 (4, 4);
        ToggleGrid grid3 (3, 4);

        BOOST_REQUIRE (grid1.sameSizeAs (grid2));
        BOOST_REQUIRE (! grid1.sameSizeAs (grid3));

        for (int i = 0; i < grid1.getNumInputs(); ++i)
            for (int o = 0; o < grid1.getNumOutputs(); ++o)
                BOOST_REQUIRE (grid1.get (i, o) == false);

        grid1.set (2, 2, true);
        BOOST_REQUIRE (grid1.get (2, 2) == true);
        BOOST_REQUIRE (grid2.get (2, 2) == false);

        grid1.swapWith (grid2);
        BOOST_REQUIRE (grid1.get (2, 2) == false);
        BOOST_REQUIRE (grid2.get (2, 2) == true);

        MatrixState matrix (6, 6);
        matrix.set (3, 3, true);
        ToggleGrid grid4 (matrix);
        BOOST_REQUIRE (grid4.getNumInputs() == matrix.getNumRows() && grid4.getNumOutputs() == matrix.getNumColumns());
        BOOST_REQUIRE (grid4.get (3, 3) == matrix.connected (3, 3));
    }
};

BOOST_AUTO_TEST_SUITE (ToggleGridTest)

BOOST_AUTO_TEST_CASE (Basics)
{
    ToggleGridUnit().runTest();
}

BOOST_AUTO_TEST_CASE (CopyConstruct)
{
    ToggleGrid source (3, 5);
    source.set (1, 4, true);

    ToggleGrid copy (source);
    BOOST_REQUIRE (copy.sameSizeAs (source));
    BOOST_REQUIRE (copy.get (1, 4));
    BOOST_REQUIRE (! copy.get (0, 0));

    copy.set (1, 4, false);
    copy.set (2, 2, true);
    BOOST_REQUIRE (source.get (1, 4));
    BOOST_REQUIRE (! source.get (2, 2));
}

BOOST_AUTO_TEST_CASE (CopyAssignSameSize)
{
    ToggleGrid source (4, 4);
    ToggleGrid dest (4, 4);
    source.set (3, 1, true);
    dest.set (0, 0, true);

    dest = source;
    BOOST_REQUIRE (dest.get (3, 1));
    BOOST_REQUIRE (! dest.get (0, 0));

    dest.set (2, 2, true);
    BOOST_REQUIRE (! source.get (2, 2));
}

BOOST_AUTO_TEST_CASE (CopyAssignDifferentSizeCopiesOverlap)
{
    ToggleGrid source (2, 2);
    source.set (1, 1, true);

    ToggleGrid dest (4, 4);
    dest.set (3, 3, true);

    dest = source;
    BOOST_REQUIRE_EQUAL (dest.getNumInputs(), 4);
    BOOST_REQUIRE_EQUAL (dest.getNumOutputs(), 4);
    BOOST_REQUIRE (dest.get (1, 1));
    BOOST_REQUIRE (! dest.get (0, 0));
    BOOST_REQUIRE (dest.get (3, 3));
}

BOOST_AUTO_TEST_CASE (MoveConstruct)
{
    ToggleGrid source (3, 5);
    source.set (2, 4, true);

    ToggleGrid moved (std::move (source));
    BOOST_REQUIRE_EQUAL (moved.getNumInputs(), 3);
    BOOST_REQUIRE_EQUAL (moved.getNumOutputs(), 5);
    BOOST_REQUIRE (moved.get (2, 4));

    BOOST_REQUIRE_EQUAL (source.getNumInputs(), 0);
    BOOST_REQUIRE_EQUAL (source.getNumOutputs(), 0);
}

BOOST_AUTO_TEST_CASE (MoveAssignSwaps)
{
    ToggleGrid a (2, 3);
    a.set (1, 2, true);

    ToggleGrid b (4, 4);
    b.set (3, 3, true);

    b = std::move (a);
    BOOST_REQUIRE_EQUAL (b.getNumInputs(), 2);
    BOOST_REQUIRE_EQUAL (b.getNumOutputs(), 3);
    BOOST_REQUIRE (b.get (1, 2));

    BOOST_REQUIRE_EQUAL (a.getNumInputs(), 4);
    BOOST_REQUIRE_EQUAL (a.getNumOutputs(), 4);
    BOOST_REQUIRE (a.get (3, 3));
}

BOOST_AUTO_TEST_CASE (MovedFromCanBeReassigned)
{
    ToggleGrid source (2, 2);
    ToggleGrid moved (std::move (source));

    ToggleGrid replacement (3, 3);
    replacement.set (2, 1, true);
    source = std::move (replacement);
    BOOST_REQUIRE_EQUAL (source.getNumInputs(), 3);
    BOOST_REQUIRE (source.get (2, 1));
}

BOOST_AUTO_TEST_SUITE_END()
