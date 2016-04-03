/*
    Buffer.cpp - This file is part of Element
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
    // This can only be called once for now, no factory
    // switching yet
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
