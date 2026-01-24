# SPDX-FileCopyrightText: 2026 Kushview, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

# The juce version to use.  This can be a git tag, hash, or branch.
set(ELEMENT_JUCE_VERSION 8.0.12)

if(NOT TARGET juce::juce_core)
    find_package(JUCE ${ELEMENT_JUCE_VERSION} CONFIG)
    if(NOT JUCE_FOUND)
        if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.28)
            FetchContent_Declare(juce
                GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
                GIT_TAG ${ELEMENT_JUCE_VERSION}
                GIT_SHALLOW ON
                EXCLUDE_FROM_ALL)
            FetchContent_MakeAvailable(juce)
        else()
            FetchContent_Declare(juce
                GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
                GIT_TAG ${ELEMENT_JUCE_VERSION}
                GIT_SHALLOW ON)
            FetchContent_Populate(juce)
            add_subdirectory(${juce_SOURCE_DIR} ${juce_BINARY_DIR} EXCLUDE_FROM_ALL)
        endif()
    endif()
endif()

# Get JUCE modules directory for manual static library setup
if(TARGET juce::juce_core)
    get_target_property(_juce_includes juce::juce_core INTERFACE_INCLUDE_DIRECTORIES)
    list(GET _juce_includes 0 JUCE_MODULES_DIR)
    message(STATUS "JUCE modules directory: ${JUCE_MODULES_DIR}")
    unset(_juce_includes)
elseif(DEFINED juce_SOURCE_DIR)
    # FetchContent path
    set(JUCE_MODULES_DIR "${juce_SOURCE_DIR}/modules")
    message(STATUS "JUCE modules directory: ${JUCE_MODULES_DIR}")
endif()

if(NOT EXISTS "${JUCE_MODULES_DIR}")
    message(FATAL_ERROR "JUCE modules directory not found: ${JUCE_MODULES_DIR}")
endif()
