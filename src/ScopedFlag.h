/*
    This file is part of Element
    Copyright (C) 2019 Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

namespace Element {

/** Utility class that will set a boolean to a a new value, then
    reset it back when this class goes out of scope 
  */
class ScopedFlag final
{
public:
    ScopedFlag() = delete;
    explicit ScopedFlag (bool& value, bool newValue) noexcept
        : valueToSet (value)
    {
        previousValue = value;
        valueToSet = newValue;
    }

    ~ScopedFlag() noexcept
    {
        valueToSet = previousValue;
    }

private:
    bool& valueToSet;
    bool previousValue;
};

}
