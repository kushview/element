#pragma once

namespace Element {

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
