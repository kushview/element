# SPDX-FileCopyrightText: 2026 Kushview, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

if(NOT TARGET juce::juce_core)
    find_package(JUCE 8.0.12 CONFIG)
    if(NOT JUCE_FOUND)
        FetchContent_Declare(juce
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG 8.0.12
            GIT_SHALLOW ON)
        FetchContent_MakeAvailable(juce)
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
