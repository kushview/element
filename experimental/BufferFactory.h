/*
    buffer-factory.hpp - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
*/

#pragma once

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
