// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>
#include <element/aligneddata.hpp>
#include <cstdint>

using element::AlignedData;

BOOST_AUTO_TEST_SUITE (AlignedDataTests)

BOOST_AUTO_TEST_CASE (default_constructor)
{
    AlignedData<16> data;
    BOOST_REQUIRE (data.data() == nullptr);
    BOOST_REQUIRE_EQUAL (data.size(), 0);
}

BOOST_AUTO_TEST_CASE (constructor_with_size)
{
    AlignedData<16> data (1024);
    BOOST_REQUIRE (data.data() != nullptr);
    BOOST_REQUIRE (data.size() >= 1024);
}

BOOST_AUTO_TEST_CASE (alignment_16_bytes)
{
    AlignedData<16> data (1024);
    void* ptr = data.data();
    
    // Check that pointer is aligned to 16-byte boundary
    uintptr_t address = reinterpret_cast<uintptr_t> (ptr);
    BOOST_REQUIRE_EQUAL (address % 16, 0);
}

BOOST_AUTO_TEST_CASE (alignment_32_bytes)
{
    AlignedData<32> data (512);
    void* ptr = data.data();
    
    // Check that pointer is aligned to 32-byte boundary
    uintptr_t address = reinterpret_cast<uintptr_t> (ptr);
    BOOST_REQUIRE_EQUAL (address % 32, 0);
}

BOOST_AUTO_TEST_CASE (alignment_64_bytes)
{
    AlignedData<64> data (256);
    void* ptr = data.data();
    
    // Check that pointer is aligned to 64-byte boundary
    uintptr_t address = reinterpret_cast<uintptr_t> (ptr);
    BOOST_REQUIRE_EQUAL (address % 64, 0);
}

BOOST_AUTO_TEST_CASE (alignment_4_bytes)
{
    AlignedData<4> data (100);
    void* ptr = data.data();
    
    // Check that pointer is aligned to 4-byte boundary
    uintptr_t address = reinterpret_cast<uintptr_t> (ptr);
    BOOST_REQUIRE_EQUAL (address % 4, 0);
}

BOOST_AUTO_TEST_CASE (move_constructor)
{
    AlignedData<16> data1 (1024);
    void* originalPtr = data1.data();
    size_t originalSize = data1.size();
    
    AlignedData<16> data2 (std::move (data1));
    
    BOOST_REQUIRE_EQUAL (data2.data(), originalPtr);
    BOOST_REQUIRE_EQUAL (data2.size(), originalSize);
    // Note: The implementation doesn't clear the moved-from object's pointer
}

BOOST_AUTO_TEST_CASE (move_assignment)
{
    AlignedData<16> data1 (1024);
    void* originalPtr = data1.data();
    size_t originalSize = data1.size();
    
    AlignedData<16> data2;
    data2 = std::move (data1);
    
    BOOST_REQUIRE_EQUAL (data2.data(), originalPtr);
    BOOST_REQUIRE_EQUAL (data2.size(), originalSize);
    // Note: The implementation doesn't clear the moved-from object's pointer
}

BOOST_AUTO_TEST_CASE (reset)
{
    AlignedData<16> data (1024);
    BOOST_REQUIRE (data.data() != nullptr);
    BOOST_REQUIRE (data.size() > 0);
    
    data.reset();
    
    BOOST_REQUIRE (data.data() == nullptr);
    BOOST_REQUIRE_EQUAL (data.size(), 0);
}

BOOST_AUTO_TEST_CASE (swap)
{
    AlignedData<16> data1 (512);
    AlignedData<16> data2 (1024);
    
    void* ptr1 = data1.data();
    void* ptr2 = data2.data();
    size_t size1 = data1.size();
    size_t size2 = data2.size();
    
    data1.swap (data2);
    
    BOOST_REQUIRE_EQUAL (data1.data(), ptr2);
    BOOST_REQUIRE_EQUAL (data1.size(), size2);
    BOOST_REQUIRE_EQUAL (data2.data(), ptr1);
    BOOST_REQUIRE_EQUAL (data2.size(), size1);
}

BOOST_AUTO_TEST_CASE (swap_with_empty)
{
    AlignedData<16> data1 (1024);
    AlignedData<16> data2;
    
    void* ptr1 = data1.data();
    size_t size1 = data1.size();
    
    data1.swap (data2);
    
    BOOST_REQUIRE (data1.data() == nullptr);
    BOOST_REQUIRE_EQUAL (data1.size(), 0);
    BOOST_REQUIRE_EQUAL (data2.data(), ptr1);
    BOOST_REQUIRE_EQUAL (data2.size(), size1);
}

BOOST_AUTO_TEST_CASE (small_allocation)
{
    AlignedData<16> data (1);
    BOOST_REQUIRE (data.data() != nullptr);
    BOOST_REQUIRE (data.size() >= 1);
    
    uintptr_t address = reinterpret_cast<uintptr_t> (data.data());
    BOOST_REQUIRE_EQUAL (address % 16, 0);
}

BOOST_AUTO_TEST_CASE (large_allocation)
{
    AlignedData<16> data (1024 * 1024); // 1 MB
    BOOST_REQUIRE (data.data() != nullptr);
    BOOST_REQUIRE (data.size() >= 1024 * 1024);
    
    uintptr_t address = reinterpret_cast<uintptr_t> (data.data());
    BOOST_REQUIRE_EQUAL (address % 16, 0);
}

BOOST_AUTO_TEST_CASE (write_and_read_data)
{
    AlignedData<16> data (100);
    char* ptr = static_cast<char*> (data.data());
    
    // Write data
    for (size_t i = 0; i < 100; ++i)
    {
        ptr[i] = static_cast<char> (i);
    }
    
    // Read and verify data
    for (size_t i = 0; i < 100; ++i)
    {
        BOOST_REQUIRE_EQUAL (ptr[i], static_cast<char> (i));
    }
}

BOOST_AUTO_TEST_CASE (different_alignments_coexist)
{
    AlignedData<4> data4 (100);
    AlignedData<8> data8 (100);
    AlignedData<16> data16 (100);
    AlignedData<32> data32 (100);
    
    uintptr_t addr4 = reinterpret_cast<uintptr_t> (data4.data());
    uintptr_t addr8 = reinterpret_cast<uintptr_t> (data8.data());
    uintptr_t addr16 = reinterpret_cast<uintptr_t> (data16.data());
    uintptr_t addr32 = reinterpret_cast<uintptr_t> (data32.data());
    
    BOOST_REQUIRE_EQUAL (addr4 % 4, 0);
    BOOST_REQUIRE_EQUAL (addr8 % 8, 0);
    BOOST_REQUIRE_EQUAL (addr16 % 16, 0);
    BOOST_REQUIRE_EQUAL (addr32 % 32, 0);
}

BOOST_AUTO_TEST_CASE (multiple_move_operations)
{
    AlignedData<16> data1 (1024);
    void* originalPtr = data1.data();
    
    AlignedData<16> data2 (std::move (data1));
    AlignedData<16> data3 (std::move (data2));
    AlignedData<16> data4 (std::move (data3));
    
    BOOST_REQUIRE_EQUAL (data4.data(), originalPtr);
    // Note: The implementation doesn't clear moved-from object pointers
}

BOOST_AUTO_TEST_CASE (reset_after_move)
{
    AlignedData<16> data1 (1024);
    AlignedData<16> data2 (std::move (data1));
    
    // Should be safe to reset moved-from object
    data1.reset();
    
    BOOST_REQUIRE (data1.data() == nullptr);
    BOOST_REQUIRE_EQUAL (data1.size(), 0);
    BOOST_REQUIRE (data2.data() != nullptr);
}

BOOST_AUTO_TEST_CASE (alignment_128_bytes)
{
    AlignedData<128> data (256);
    void* ptr = data.data();
    
    // Check that pointer is aligned to 128-byte boundary (common cache line size)
    uintptr_t address = reinterpret_cast<uintptr_t> (ptr);
    BOOST_REQUIRE_EQUAL (address % 128, 0);
}

BOOST_AUTO_TEST_SUITE_END()
