# SPDX-FileCopyrightText: 2026 Kushview, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(sol2 4.0.0 CONFIG)
if(NOT sol2_FOUND)
    FetchContent_Declare(sol2
        GIT_REPOSITORY https://github.com/ThePhD/sol2.git
        GIT_TAG c1f95a773c6f8f4fde8ca3efe872e7286afe4444
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(sol2)
endif()
