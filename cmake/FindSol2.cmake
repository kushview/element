# SPDX-FileCopyrightText: 2026 Kushview, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(sol2 4.0.0 CONFIG)
if(NOT sol2_FOUND)
    set(ELEMENT_SOL2_REPO "https://github.com/ThePhD/sol2.git")
    set(ELEMENT_SOL2_REVISION "c1f95a773c6f8f4fde8ca3efe872e7286afe4444")
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.28)
        FetchContent_Declare(sol2
            GIT_REPOSITORY ${ELEMENT_SOL2_REPO}
            GIT_TAG ${ELEMENT_SOL2_REVISION}
            GIT_SHALLOW ON
            EXCLUDE_FROM_ALL)
        FetchContent_MakeAvailable(sol2)
    else()
        FetchContent_Declare(sol2
            GIT_REPOSITORY ${ELEMENT_SOL2_REPO}
            GIT_TAG ${ELEMENT_SOL2_REVISION}
            GIT_SHALLOW ON)
        FetchContent_Populate(sol2)
        add_subdirectory(${sol2_SOURCE_DIR} ${sol2_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()
endif()
