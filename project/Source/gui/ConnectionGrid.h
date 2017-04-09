

#ifndef EL_CONNECTION_GRID_H
#define EL_CONNECTION_GRID_H

#include "element/Juce.h"

namespace Element {
    
    class ConnectionGrid :  public PatchMatrixComponent
    {
    public:
        ConnectionGrid();
        ~ConnectionGrid();
        
        void paintMatrixCell (Graphics& g, const int width, const int height,
                              const int row, const int column) override;
        int getNumColumns() override;
        int getNumRows() override;
    };
}

#endif  // EL_CONNECTION_GRID_H
