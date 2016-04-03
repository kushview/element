/*
    Buffer.h - This file is part of Element
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

#ifndef ELEMENT_BUFFER_H
#define ELEMENT_BUFFER_H

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

#endif /* ELEMENT_BUFFER_H */
