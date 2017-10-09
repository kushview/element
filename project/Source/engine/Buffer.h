/*
    Buffer.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
*/

#pragma once

#if ELEMENT_BUFFER_FACTORY
class Buffer;
class BufferFactory;

void intrusive_ptr_add_ref (Buffer* b);
void intrusive_ptr_release (Buffer* b);

class Buffer
{
public:

    enum DataType {
        controlData,
        audioData,
        midiData,
        atomData,
        eventData
    };

    virtual ~Buffer();

    virtual void clear() noexcept = 0;
    virtual void clear (int start, int end) = 0;

    bool isManaged() const { return factory != nullptr; }

    uint32 getCapacity() const { return capacity; }
    DataType getType()   const { return dataType; }
    uint32 getSubType()  const { return subType; }

    void* getChannelData (int channel = 0);

protected:

    explicit Buffer (DataType dataType, uint32 subtype = 0);

private:

    void attach (BufferFactory* owner);
    void recycle();

    BufferFactory*      factory;
    std::atomic<uint32> refs;
    DataType            dataType;
    uint32              subType;
    uint32              capacity;
    Buffer*             next;

    friend class BufferFactory;
    friend void intrusive_ptr_add_ref (Buffer*);
    friend void intrusive_ptr_release (Buffer*);
};

class BufferRef : public boost::intrusive_ptr<Buffer>
{
public:
    BufferRef() :  boost::intrusive_ptr<Buffer>() { }
    BufferRef (Buffer* buf)
        : boost::intrusive_ptr<Buffer> (buf, true) { }
};
#endif
