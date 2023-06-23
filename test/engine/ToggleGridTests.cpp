

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
                BOOST_REQUIRE (grid1.get (i, 0) == false);

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

BOOST_AUTO_TEST_SUITE (ToggleGridTests)

BOOST_AUTO_TEST_CASE (Basics)
{
    ToggleGridUnit().runTest();
}

BOOST_AUTO_TEST_SUITE_END()
