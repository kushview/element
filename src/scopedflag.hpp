// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

namespace element {

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

} // namespace element
