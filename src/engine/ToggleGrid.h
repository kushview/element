#pragma once

namespace Element {

/** Similar to a kv::MatrixState but is intended to be used in a realtime context */
class ToggleGrid
{
public:
    /** This ctor will allocate: DO NOT create these on the stack in a realtime 
        thread, use two instances ToggleGrid::swapWith or operator= instead */
    explicit ToggleGrid (const int ins = 4, const int outs = 4)
        : numIns (ins), numOuts (outs)
    {
        jassert (ins > 0 && outs > 0);
        toggles = new bool* [numIns];
        for (int i = 0; i < numIns; ++i)
            toggles[i] = new bool [numOuts];
        clear();
    }

    ~ToggleGrid() noexcept 
    {
        for (int i = 0; i < numIns; ++i)
            delete [] toggles [i];
        delete [] toggles;
    }

    inline bool sameSizeAs (const ToggleGrid& other) const noexcept
    {
        return numIns == other.numIns && numOuts == other.numOuts;
    }

    inline void clear() noexcept
    {
        for (int i = 0; i < numIns; ++i)
            for (int o = 0; o < numOuts; ++o)
                toggles[i][o] = false;
    }

    inline bool get (const int in, const int out) const noexcept
    {
        jassert (isPositiveAndBelow (in, numIns) && isPositiveAndBelow (out, numOuts));
        return toggles[in][out];
    }

    inline void set (const int in, const int out, const bool value) noexcept
    {
        jassert (isPositiveAndBelow (in, numIns) && isPositiveAndBelow (out, numOuts));
        toggles[in][out] = value;
    }

    inline int getNumInputs() const noexcept    { return numIns; }
    inline int getNumOutputs() const noexcept   { return numOuts; }

    inline void swapWith (ToggleGrid& other) noexcept
    {
        std::swap (toggles, other.toggles);
    }

    ToggleGrid& operator= (const ToggleGrid& other)
    {
        if (sameSizeAs (other))
        {
            for (int i = 0; i < numIns; ++i)
                for (int o = 0; o < numOuts; ++o)
                    toggles[i][o] = other.toggles[i][o];
        }
        else
        {
            for (int i = 0; i < jmin (numIns, other.numIns); ++i)
                for (int o = 0; o < jmin (numOuts, other.numOuts); ++o)
                    toggles[i][o] = other.toggles[i][o];
        }

        return *this;
    }

private:
    int numIns, numOuts;
    bool** toggles = nullptr;
};

}
