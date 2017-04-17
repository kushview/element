/*
    This file is part of the element modules for the JUCE Library
    Copyright (C) 2016  Kushview, LLC.  All rights reserved.
*/

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
    class Controls; Controls* controls;
    class Sources; Sources* sources;
    class Destinations; Destinations* destinations;
    class ViewPort; ViewPort* view; // not used
    class Quads; ScopedPointer<QuadrantLayout> quads;
};

}

#endif  // EL_CONNECTION_GRID_H
