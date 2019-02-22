/*
    Buffer.cpp - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
*/

#if ELEMENT_BUFFER_FACTORY

Buffer::Buffer (DataType dataType_, uint32 subType_)
    : factory (nullptr),
      refs (0),
      dataType (dataType_),
      subType (subType_),
      capacity (0),
      next (nullptr)
{ }

Buffer::~Buffer() { }

void Buffer::attach (BufferFactory* owner)
{
    jassert (factory == nullptr && owner != nullptr);
    factory = owner;
}

void Buffer::recycle()
{
    if (isManaged())
        factory->recycle (this);
}

void intrusive_ptr_add_ref (Buffer* b)
{
    if (b->isManaged())
        ++b->refs;
}

void intrusive_ptr_release (Buffer* b)
{
    if (b->isManaged())
        if (--b->refs == 0)
            b->recycle();
}

#endif
