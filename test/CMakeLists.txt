# SPDX-License-Identifier: GPL-3.0-or-later

file(GLOB_RECURSE TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

juce_add_console_app(test_element)
target_sources(test_element PRIVATE ${TEST_SOURCES})
target_link_libraries(test_element PRIVATE kv::element)
target_include_directories(test_element 
    PRIVATE 
        "${CMAKE_CURRENT_SOURCE_DIR}"
        ${Boost_INCLUDE_DIRS})

target_compile_definitions(test_element
    PRIVATE
        EL_TEST_SOURCE_ROOT="${CMAKE_SOURCE_DIR}")

# Register as a CTest test
add_test(NAME element_unit_tests COMMAND test_element)
