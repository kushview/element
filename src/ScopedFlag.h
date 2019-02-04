#pragma once

namespace Element {

/** Utility class that will set a boolean to a a new value, then
    reset it back when this class goes out of scope 
  */
class ScopedFlag
{
public:
    ScopedFlag() = delete;
    explicit ScopedFlag (bool& value, bool newValue)
        : valueToSet (value)
    {
        previousValue = value;
        valueToSet = newValue;
    }

    ~ScopedFlag()
    {
        valueToSet = previousValue;
    }

private:
    bool& valueToSet;
    bool previousValue;
};

}
