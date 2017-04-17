
#ifndef EL_CONNECTION_GRID_H
#define EL_CONNECTION_GRID_H

#include "element/Juce.h"

namespace Element {
    
    class ConnectionGrid : public Component
    {
    public:
        ConnectionGrid();
        ~ConnectionGrid();

        void resized() override;
        
    private:
        class PatchMatrix; PatchMatrix* matrix;
        class ViewPort; ViewPort* view;
        class Quads; ScopedPointer<QuadrantLayout> quads;
    };
}

#endif  // EL_CONNECTION_GRID_H
