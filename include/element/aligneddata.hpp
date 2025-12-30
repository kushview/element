// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <memory>

namespace element {

/** A template class for managing memory with specific alignment requirements.
    
    AlignedData allocates a block of memory and ensures that the usable portion
    is aligned to a specific boundary. This is useful for SIMD operations, cache
    line optimization, or interfacing with hardware that requires specific memory
    alignment.
    
    @tparam Alignment  The required alignment boundary in bytes (must be a power of 2)
    
    Example usage:
    @code
    // Create 1024 bytes aligned to 16-byte boundary
    AlignedData<16> buffer(1024);
    void* ptr = buffer.data();
    size_t size = buffer.size();
    @endcode
*/
template <size_t Alignment>
class AlignedData {
public:
    /** Constructs an empty AlignedData object with no allocated memory. */
    AlignedData() = default;

    /** Allocates memory with the specified size and alignment.
        
        @param size  The number of bytes to allocate (actual allocation may be larger
                     to accommodate alignment requirements)
    */
    explicit AlignedData (size_t size)
        : _data (new char[size + Alignment]),
          _ptr (_data.get()),
          _space (size + Alignment)
    {
        _ptr = std::align (Alignment, size, _ptr, _space);
    }

    /** Move assignment operator.
        
        Transfers ownership of the allocated memory from another AlignedData object.
        
        @param o  The source object to move from
        @return   Reference to this object
    */
    AlignedData& operator= (AlignedData&& o) noexcept
    {
        _data = std::move (o._data);
        _ptr = std::move (o._ptr);
        _space = std::move (o._space);
        return *this;
    }

    /** Move constructor.
        
        Creates a new AlignedData by transferring ownership from another object.
        
        @param o  The source object to move from
    */
    AlignedData (AlignedData&& o) noexcept
    {
        *this = std::move (o);
    }

    /** Returns a pointer to the aligned memory region.
        
        @return  A pointer to the aligned data, or nullptr if no memory is allocated
    */
    constexpr void* data() const noexcept { return _ptr; }
    
    /** Returns the size of the usable aligned memory region.
        
        Note: This may be less than the originally requested size due to alignment.
        
        @return  The size in bytes of the aligned region
    */
    constexpr size_t size() const noexcept { return _space; }

    /** Releases the allocated memory and resets the object to an empty state. */
    void reset() noexcept
    {
        _ptr = nullptr;
        _data.reset();
        _space = 0;
    }

    /** Swaps the contents of this AlignedData with another.
        
        @param b  The AlignedData object to swap with
    */
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

} // namespace element
