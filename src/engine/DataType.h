/*
    DataType.h - This file is part of Element
    Copyright (C) 2014-2019  Kushview, LLC.  All rights reserved.

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

#pragma once

class DataType {
public:
    enum ID {
        Audio,
        MIDI,
        Unknown
    };

    inline static int32 numTypes() { return Unknown; }

    inline DataType() : type (Unknown) {}
    inline DataType (const ID& i) : type (i) {}
    template<typename INT> inline
    DataType (const INT& i)
        : type (isPositiveAndBelow (i, static_cast<INT> (Unknown)) ? static_cast<ID> (i) : Unknown) {}
    inline DataType& operator= (const DataType& other) { type = other.type; return *this; }

private:
    ID type;
};
