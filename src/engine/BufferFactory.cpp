/*
    BufferFactory.cpp - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
*/

#if 0

#include "engine/BufferFactory.h"

namespace Element {
BufferFactory::BufferFactory()
    : freeAudio (nullptr),
      freeControl (nullptr),
      freeMidi (nullptr),
      freeAtom (nullptr)
{

}

BufferFactory::~BufferFactory()
{
    destroyFreeList (freeAtom.load());
    destroyFreeList (freeControl.load());
    destroyFreeList (freeAudio.load());
    destroyFreeList (freeMidi.load());
}

uint32 BufferFactory::audioBufferSize (int numSamples)
{
    uint32 val = 1;
    while (val < numSamples)
        val *= 2;
    return val;
}

BufferRef BufferFactory::createBuffer (Buffer::DataType type, uint32 capacity, int channels)
{
    if (capacity == 0)
        capacity = defaultCapacity (type);
    else if (type == Buffer::controlData)
        capacity = jmax ((int) capacity, 1);
    else if (type == Buffer::audioData)
        capacity = jmax (capacity, defaultCapacity (Buffer::audioData));

    Buffer* buffer (nullptr);
#if 0
    switch (type)
    {
        case Buffer::audioData:
        case Buffer::controlData:
            buffer = new AudioSampleBuffer (channels, (int) capacity);
        case Buffer::midiData:
            buffer = new MidiBuffer();
            ((MidiBuffer*)buffer)->ensureSize (capacity);
            break;
        case Buffer::atomData:
        default:
            break;
    }
#endif
    // Juce buffer types do not have ctors that set the factory member
    if (buffer)
        buffer->attach (this);

    return BufferRef (buffer);
}

uint32 BufferFactory::defaultCapacity (Buffer::DataType type) const
{
    switch (type)
    {
        case Buffer::controlData:  return 1;
        case Buffer::audioData:    return 1024;
        case Buffer::midiData:     return 1024;
        case Buffer::atomData:     return 4096;
        case Buffer::eventData:    return 4096;
    }
    return 1;
}

BufferRef
BufferFactory::getBuffer (Buffer::DataType type, int capacity,
                          bool realTime, bool forceCreate)
{
    BufferList& headPtr = getFreeList (type);
    Buffer*               tryHead = nullptr;

    if (! forceCreate)
    {
        Buffer* next;
        do {
            tryHead = headPtr.load();
            if (! tryHead)
                break;
            next = tryHead->next;
        } while (! headPtr.compare_exchange_weak (tryHead, next));
    }

    if (! tryHead)
    {
        if (! realTime)
            return createBuffer (type, capacity);
        else
            return BufferRef();
    }

    tryHead->next = nullptr;
    jassert (type == tryHead->getType());
    return BufferRef (tryHead);
}

void BufferFactory::destroyFreeList (Buffer* listHead)
{
    while (listHead)
    {
        Buffer* next = listHead->next;
        delete listHead;
        listHead = next;
    }
}

BufferFactory::BufferList& BufferFactory::getFreeList (Buffer::DataType type)
{
    switch (type)
    {
        case Buffer::controlData:  return freeControl;
        case Buffer::audioData:    return freeAudio;
        case Buffer::midiData:     return freeMidi;
        case Buffer::atomData:     return freeAtom;
        case Buffer::eventData:     return freeAtom;
    }
}

uint32 BufferFactory::reserve (const Buffer::DataType type,
                               const uint32 bufferCount,
                               const uint32 /*subType*/)
{
    uint32 numCreated = 0;
    for (uint32 i = 0; i < bufferCount; ++i)
        numCreated += (nullptr == createBuffer (type, defaultCapacity (type))) ? 0 : 1;

    jassert (bufferCount == numCreated);
    return numCreated;
}

void BufferFactory::setBlockSize (int size)
{
    blockSize = size;
}

void BufferFactory::setEventSize (int size)
{
    eventSize = size;
}


void BufferFactory::recycle (Buffer* buffer)
{
    BufferList& headPtr = getFreeList (buffer->getType());

    if (buffer->getSubType() > 0)
        { /* no yet supported */ }

    Buffer* tryHead (nullptr);
    do {
        tryHead = headPtr.load();
        buffer->next = tryHead;
    } while (headPtr.compare_exchange_weak (tryHead, buffer));
}

}

#endif
