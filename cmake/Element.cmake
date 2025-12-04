# SPDX-License-Identifier: GPL-3.0-or-later

option(ELEMENT_USE_VST2 "Build with VST2 support" OFF)

if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(USER_HOME_DIRECTORY "$ENV{USERPROFILE}")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin") # Darwin is macOS
    set(USER_HOME_DIRECTORY "$ENV{HOME}")
else()
    message(WARNING "Unsupported operating system for determining home directory.")
    set(USER_HOME_DIRECTORY "") # Set to empty or a default value
endif()

message(STATUS "User Home Directory: ${USER_HOME_DIRECTORY}")
