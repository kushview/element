
#include "gui/ConnectionGrid.h"

namespace Element {
    static const int gridPadding = 1;
    
    ConnectionGrid::ConnectionGrid()
        : matrix (8, 8)
    {
        for (int i = 0 ; i < 8; ++i)
            matrix.connect (i, i);
    }
    
    ConnectionGrid::~ConnectionGrid() { }
    
    int ConnectionGrid::getNumColumns()     { return matrix.getNumColumns(); }
    int ConnectionGrid::getNumRows()        { return matrix.getNumRows(); }
    
    void ConnectionGrid::matrixCellClicked (const int row, const int col, const MouseEvent& ev)
    {
        matrix.toggleCell (row, col);
        repaint();
    }
    
    void ConnectionGrid::paintMatrixCell (Graphics& g, const int width, const int height,
                                          const int row, const int column)
    {
        g.setColour (matrix.isCellToggled (row, column) ?
                     Colour (Element::LookAndFeel_E1::defaultMatrixCellOnColor) :
                     Colour (Element::LookAndFeel_E1::defaultMatrixCellOffColor));
        g.fillRect (0, 0, width - gridPadding, height - gridPadding);
    }
}
