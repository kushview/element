# SPDX-FileCopyrightText: 2026 Kushview, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

option(ELEMENT_BUILD_PLUGINS "Build the plugin versions of Element" OFF)
option(ELEMENT_BUILD_TESTS "Build the unit tests" ON)
option(ELEMENT_ENABLE_ASIO "Build with ASIO support" OFF)
option(ELEMENT_ENABLE_VST2 "Build with VST2 support" OFF)
option(ELEMENT_ENABLE_LTO "Build with Link Time Optimization" OFF)
option(ELEMENT_ENABLE_UPDATER "Build with updater support" OFF)

set(ELEMENT_APP_PLIST_TO_MERGE "")

if(APPLE)
    enable_language(OBJC)
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

# Enable ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Found ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
else()
    message(STATUS "ccache not found")
endif()

include(GNUInstallDirs)
include(FetchContent)

# Fetch ASIO
if(WIN32 AND ELEMENT_ENABLE_ASIO)
    FetchContent_Declare(ASIOSDK
        GIT_REPOSITORY https://github.com/audiosdk/asio.git
        GIT_TAG 496a0765b8bb9c26f764f22f9a9712a937177db2)
    FetchContent_MakeAvailable(ASIOSDK)
endif()

# JUCE VST2 SDK path setup
if(ELEMENT_ENABLE_VST2)
    set(JUCE_GLOBAL_VST2_SDK_PATH "${USER_HOME_DIRECTORY}/SDKs/vstsdk2.4")
    message(STATUS "VST2 SDK Path: ${JUCE_GLOBAL_VST2_SDK_PATH}")
endif()

# Install a plugin by target name
function(element_install_plugin tgt)
    if(LINUX OR WIN32)
        install(TARGETS ${tgt}_CLAP DESTINATION "${CMAKE_INSTALL_LIBDIR}/clap")
        install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${tgt}_artefacts/$<CONFIG>/LV2/"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/lv2")
        install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${tgt}_artefacts/$<CONFIG>/VST3/"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/vst3")
    elseif(APPLE)
        install(TARGETS "${tgt}_AU"   LIBRARY DESTINATION "Library/Audio/Plug-Ins/Components")
        install(TARGETS "${tgt}_CLAP" LIBRARY DESTINATION "Library/Audio/Plug-Ins/CLAP")
        install(TARGETS "${tgt}_VST3" LIBRARY DESTINATION "Library/Audio/Plug-Ins/VST3")
        # LV2 on macOS isn't a real library bundle.
        install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${tgt}_artefacts/$<CONFIG>/LV2/"
            DESTINATION "Library/Audio/Plug-Ins/LV2")
    endif()

    if(ELEMENT_ENABLE_VST2)
        if(LINUX OR WIN32)
            install(TARGETS ${tgt}_VST DESTINATION "${CMAKE_INSTALL_LIBDIR}/vst")
        elseif(APPLE)
            install(TARGETS "${tgt}_VST" LIBRARY DESTINATION "Library/Audio/Plug-Ins/VST")
        endif()
    endif()
endfunction()
