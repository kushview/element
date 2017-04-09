
#include "gui/ConnectionGrid.h"

namespace Element {
    ConnectionGrid::ConnectionGrid() { }
    
    ConnectionGrid::~ConnectionGrid() { }
    
    int ConnectionGrid::getNumColumns()
    {
        return 4;
    }
    
    int ConnectionGrid::getNumRows()
    {
        return 4;
    }
    
    void ConnectionGrid::paintMatrixCell (Graphics& g, const int width, const int height,
                                          const int row, const int column)
    {
        if (row == 1 && column == 1)
        {
            g.setColour(Colours::red);
            g.fillRect (0, 0, width, height);
        }
    }
}
