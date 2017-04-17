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

    void paint (Graphics&) override;
    void resized() override;
    
private:
    friend class PatchMatrix;
    class PatchMatrix; PatchMatrix* matrix;
    
    friend class Controls;
    class Controls; Controls* controls;
    
    friend class Sources;
    class Sources; Sources* sources;
    
    friend class Destinations;
    class Destinations; Destinations* destinations;
    
    friend class Quads;
    class Quads; ScopedPointer<QuadrantLayout> quads;
};

}

#endif  // EL_CONNECTION_GRID_H
