/*
    This file is part of the Kushview Modules for JUCE
    Copyright (c) 2014-2019  Kushview, LLC.  All rights reserved.

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

#include "JuceHeader.h"

namespace element {

struct DockPlacement
{
    enum Type
    {
        Top = 0,
        Left,
        Bottom,
        Right,
        Center,
        Floating
    };

    static const int numTypes = 6;

    DockPlacement() = default;
    DockPlacement (const Type& t) : type (t) {}
    DockPlacement (const DockPlacement& o) { operator= (o); }
    template <typename IntType>
    DockPlacement (const IntType& t) : type (static_cast<int> (t))
    {
        jassert (isValid (type));
    }

    ~DockPlacement() = default;

    Type getType() const
    {
        jassert (isValid());
        return static_cast<Type> (type);
    }

    inline static bool isValid (const int t) { return t >= Top && t < numTypes; }
    inline bool isValid() const { return isValid (type); }
    inline static bool isDirectional (const int t) { return t == Top || t == Left || t == Bottom || t == Right; }
    inline bool isDirectional() const { return isDirectional (type); }
    inline static bool isVertical (const int t) { return t == Top || t == Bottom; }
    inline bool isVertical() const { return isVertical (type); }

    inline bool isTop() const { return type == Top; }
    inline bool isLeft() const { return type == Left; }
    inline bool isBottom() const { return type == Bottom; }
    inline bool isRight() const { return type == Right; }
    inline bool isCenter() const { return type == Center; }
    inline bool isFloating() const { return type == Floating; }

    inline int toInt() const { return type; }
    inline static const String& toString (const int t)
    {
        static String _types[] = { "top", "left", "bottom", "right", "center", "floating", "" };
        return isValid (t) ? _types[t] : _types[numTypes];
    }
    inline const String& toString() const { return toString (type); }

    inline operator int() const { return toInt(); }

    inline bool operator== (const DockPlacement& o) const { return this->type == o.type; }
    inline bool operator== (const DockPlacement::Type& t) { return this->type == t; }
    inline bool operator== (const int& t) { return this->type == t; }
    inline bool operator== (const uint32& t) { return this->type == static_cast<int> (t); }
    inline bool operator!= (const DockPlacement& o) const { return this->type != o.type; }
    inline bool operator!= (const DockPlacement::Type& t) { return this->type != t; }
    inline bool operator!= (const int& t) { return this->type != t; }
    inline bool operator!= (const uint32& t) { return this->type != static_cast<int> (t); }

    inline DockPlacement& operator= (const DockPlacement::Type& t)
    {
        this->type = t;
        return *this;
    }
    inline DockPlacement& operator= (const DockPlacement& o)
    {
        this->type = o.type;
        return *this;
    }

private:
    int type = Top;
};

} // namespace element
