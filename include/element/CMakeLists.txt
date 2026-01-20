# SPDX-FileCopyrightText: 2026 Kushview, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

# Generate a version header in a specific path (build/include/element/version.h)

# Configurable base commit/tag for build numbering
set(ELEMENT_GIT_BASE "72f454b409ee0e719b72a87d3a0209339952814a" CACHE STRING "Base commit/tag for build numbering")

# Allow build number override (e.g., from CI)
set(ELEMENT_BUILD_NUMBER "" CACHE STRING "Computed build number (overrideable)")

# Only run git commands if git is found and .git directory exists
find_package(Git)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Get full commit hash
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --verify HEAD
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE ELEMENT_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE GIT_HASH_RESULT)
    
    if(NOT GIT_HASH_RESULT EQUAL 0 OR NOT ELEMENT_GIT_HASH)
        set(ELEMENT_GIT_HASH "unknown")
    endif()
    
    # Get short hash
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE ELEMENT_GIT_SHORT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    
    if(NOT ELEMENT_GIT_SHORT_HASH)
        string(SUBSTRING ${ELEMENT_GIT_HASH} 0 8 ELEMENT_GIT_SHORT_HASH)
    endif()
    
    # Compute build number (commit count since base) if not overridden
    if(NOT ELEMENT_BUILD_NUMBER)
        execute_process(COMMAND ${GIT_EXECUTABLE} rev-list ${ELEMENT_GIT_BASE}..HEAD --count
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            OUTPUT_VARIABLE ELEMENT_BUILD_NUMBER
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            RESULT_VARIABLE GIT_COUNT_RESULT)
        
        if(NOT GIT_COUNT_RESULT EQUAL 0 OR NOT ELEMENT_BUILD_NUMBER)
            set(ELEMENT_BUILD_NUMBER 0)
        endif()
    endif()
else()
    # Git not available or not a git repository - use defaults
    set(ELEMENT_GIT_HASH "unknown")
    set(ELEMENT_GIT_SHORT_HASH "unknown")
    if(NOT ELEMENT_BUILD_NUMBER)
        set(ELEMENT_BUILD_NUMBER 0)
    endif()
endif()

set(ELEMENT_VERSION_STRING ${PROJECT_VERSION_FULL})

set(GENERATED_INCLUDE_DIR ${CMAKE_BINARY_DIR}/include)
set(GENERATED_INCLUDE_DIR ${CMAKE_BINARY_DIR}/include PARENT_SCOPE)
file(MAKE_DIRECTORY ${GENERATED_INCLUDE_DIR}/element)

configure_file(${PROJECT_SOURCE_DIR}/cmake/version.txt.in
    ${CMAKE_BINARY_DIR}/version.txt @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${GENERATED_INCLUDE_DIR}/element/version.h @ONLY)
