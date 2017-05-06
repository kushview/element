/*
    buffer-factory.hpp - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.

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

#ifndef ELEMENT_BUFFER_FACTORY_H
#define ELEMENT_BUFFER_FACTORY_H

#include "ElementApp.h"
#include "engine/Buffer.h"

namespace Element {

class BufferFactory
{
public:
    BufferFactory();
    ~BufferFactory();

    uint32 audioBufferSize (int numSamples);
    uint32 defaultCapacity (Buffer::DataType type) const;

    BufferRef getBuffer (Buffer::DataType type, int capacity = 0,
                         bool realTime = false, bool forceCreate = false);

    uint32 reserve (Buffer::DataType type, uint32 bufferCount, uint32 subType = 0);
    void setBlockSize (int blockSize);
    void setEventSize (int eventSize);

private:
    typedef std::atomic<Buffer*> BufferList;

    BufferRef createBuffer (Buffer::DataType type, uint32 capacity = 0, int channels = 1);
    BufferList& getFreeList (Buffer::DataType type);
    void destroyFreeList (Buffer* list);
    void recycle (Buffer* buffer);

    int blockSize;
    int eventSize;

    BufferList freeAudio;
    BufferList freeControl;
    BufferList freeMidi;
    BufferList freeAtom;

    friend class Buffer;

};
}
#endif // ELEMENT_BUFFER_FACTORY_H
