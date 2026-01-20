# SPDX-FileCopyrightText: 2026 Kushview, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(JUCE 8.0.12 CONFIG)
if(NOT JUCE_FOUND)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.12
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()
