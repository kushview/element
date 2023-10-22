// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <memory>

namespace element {

template <size_t Alignment>
class AlignedData {
public:
    AlignedData() = default;

    explicit AlignedData (size_t size)
        : _data (new char[size + Alignment]),
          _ptr (_data.get()),
          _space (size + Alignment)
    {
        _ptr = std::align (Alignment, size, _ptr, _space);
    }

    AlignedData& operator= (AlignedData&& o) noexcept
    {
        _data = std::move (o._data);
        _ptr = std::move (o._ptr);
        _space = std::move (o._space);
        return *this;
    }

    AlignedData (AlignedData&& o) noexcept
    {
        *this = std::move (o);
    }

    constexpr void* data() const noexcept { return _ptr; }
    constexpr size_t size() const noexcept { return _space; }

    void reset() noexcept
    {
        _ptr = nullptr;
        _data.reset();
        _space = 0;
    }

    inline void swap (AlignedData& b) noexcept
    {
        _data.swap (b._data);
        std::swap (_ptr, b._ptr);
        std::swap (_space, b._space);
    }

private:
    std::unique_ptr<char[]> _data;
    void* _ptr = nullptr;
    size_t _space = 0;
};

#if 0
template <size_t Alignment>
static AlignedData<Alignment> grow (SingleSizeAlignedStorage<Alignment> storage, size_t size)
{
    if (size <= storage.size())
        return storage;

    AlignedData<Alignment> newStorage { jmax (size, (storage.size() * 3) / 2) };
    std::memcpy (newStorage.data(), storage.data(), storage.size());
    return newStorage;
}
#endif

}
