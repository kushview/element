# SPDX-License-Identifier: GPL-3.0-or-later

option(ELEMENT_BUILD_PLUGINS "Build the plugin versions of Element" OFF)
option(ELEMENT_BUILD_TESTS "Build the unit tests" ON)
option(ELEMENT_ENABLE_ASIO "Build with ASIO support" OFF)
option(ELEMENT_ENABLE_VST2 "Build with VST2 support" OFF)
option(ELEMENT_ENABLE_LTO "Build with Link Time Optimization" OFF)

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
        # install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${tgt}_artefacts/$<CONFIG>/CLAP/"
        #     DESTINATION "Library/Audio/Plug-Ins/CLAP")
        # install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${tgt}_artefacts/$<CONFIG>/LV2/"
        #     DESTINATION "Library/Audio/Plug-Ins/LV2")
        # install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${tgt}_artefacts/$<CONFIG>/VST3/"
        #     DESTINATION "Library/Audio/Plug-Ins/VST3")
        # install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${tgt}_artefacts/$<CONFIG>/AU/"
        #     DESTINATION "Library/Audio/Plug-Ins/Components")
    endif()

    if(ELEMENT_ENABLE_VST2)
        if(LINUX OR WIN32)
            install(TARGETS ${tgt}_VST DESTINATION "${CMAKE_INSTALL_LIBDIR}/vst")
        elseif(APPLE)
            # install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${tgt}_artefacts/$<CONFIG>/VST/"
            #     DESTINATION "Library/Audio/Plug-Ins/VST")
        endif()
    endif()
endfunction()
