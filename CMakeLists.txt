# Element CMakeLists.txt
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.22)
project(element VERSION 1.0.0)
set(PROJECT_VERSION_SUFFIX "b2")
set(PROJECT_VERSION_FULL "${PROJECT_VERSION}${PROJECT_VERSION_SUFFIX}")

include(cmake/Element.cmake)

add_subdirectory(deps/sol2 EXCLUDE_FROM_ALL)

# JUCE VST2 SDK setup
if(ELEMENT_ENABLE_VST2)
    set(JUCE_GLOBAL_VST2_SDK_PATH "${USER_HOME_DIRECTORY}/SDKs/vstsdk2.4")
endif()

include(cmake/FindJack.cmake)

add_subdirectory(deps/juce EXCLUDE_FROM_ALL)

# Global JUCE properties
set_directory_properties(PROPERTIES JUCE_COMPANY_NAME "Kushview")
set_directory_properties(PROPERTIES JUCE_COMPANY_COPYRIGHT "Kushview, LLC")
set_directory_properties(PROPERTIES JUCE_COMPANY_WEBSITE "https://kushview.net")
set_directory_properties(PROPERTIES JUCE_COMPANY_EMAIL "support@kushview.net")

# CLAP Setup
set(CMAKE_POLICY_DEFAULT_CMP0177 OLD)
add_subdirectory(deps/clap-juce-extensions EXCLUDE_FROM_ALL)

# BOOST Setup
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.30.0")
    cmake_policy(SET CMP0167 OLD)
endif()

find_package(Boost 1.74.0)
if(NOT Boost_FOUND)
    message(" -- Checking user SDK's for boost")
    set(Boost_ROOT "${USER_HOME_DIRECTORY}/SDKs/include")
    find_package(Boost 1.80.0 REQUIRED)
endif()

# Element static library
add_library(element STATIC)
add_library(kv::element ALIAS element)

set_target_properties(element PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(element
    PUBLIC
        include
        src
        src/lua/sol
        src/lua/src
        deps/clap-juce-extensions/clap-libs/clap/include
        deps/clap-juce-extensions/clap-libs/clap-helpers/include
        deps/lvtk/include
        ${Boost_INCLUDE_DIRS}
)

add_subdirectory(data)
add_subdirectory(scripts)
add_subdirectory(src/el)
add_subdirectory(include/element)

file(GLOB_RECURSE ELEMENT_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

target_sources(element PRIVATE ${ELEMENT_SOURCES})

if(APPLE)
    target_sources(element PRIVATE src/ui/nsviewwithparent.mm)
endif()

target_compile_definitions(element PUBLIC BOOST_ALL_NO_LIB=1)

target_compile_options(element PRIVATE ${SUIL_CFLAGS_OTHER})

if(JACK_FOUND)
    message(STATUS "JACK audio driver enabled")
    target_compile_definitions(element PUBLIC ELEMENT_USE_JACK=1)
    target_include_directories(element PUBLIC ${JACK_INCLUDE_DIRS})
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(element PRIVATE -Wno-sign-conversion -Wno-switch-enum -Wno-attributes)
elseif(MSVC)
    # MSVC doesn't support -Wno-sign-conversion; disable a relevant warning if needed
    # e.g. /wd4244 or /wd4267 depending on the warning you want to suppress
    target_compile_options(element PRIVATE /wd4244)
endif()

target_compile_definitions(element
    PUBLIC
        DONT_SET_USING_JUCE_NAMESPACE=1
        JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=1
        JUCE_LOAD_CURL_SYMBOLS_LAZILY=1
        JUCE_PLUGINHOST_LV2=1
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_AU=1
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(element
    PUBLIC
        juce::juce_core
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_osc
        juce::juce_dsp
        juce::juce_data_structures
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_processors
        juce::juce_audio_formats
        juce::juce_audio_utils
        element-binarydata
        element-luascripts
        element-luamods
        sol2::sol2
        juce::juce_recommended_config_flags
    )

if(ELEMENT_ENABLE_LTO)
    message("Link Time Optimization enabled")
    target_link_libraries(element PUBLIC juce::juce_recommended_lto_flags)
endif()

if(WIN32 AND ELEMENT_ENABLE_ASIO)
    message("ASIO audio device support enabled")
    target_include_directories(element PUBLIC "${USER_HOME_DIRECTORY}/SDKs/ASIOSDK2.3/common")
    target_compile_definitions(element PUBLIC JUCE_ASIO=1)
endif()

# VST2 enable host support
if(ELEMENT_ENABLE_VST2)
    message("VST2 Plugin support enabled")
    target_compile_definitions(element PUBLIC JUCE_PLUGINHOST_VST=1)
    target_link_libraries(element PUBLIC juce_vst2_sdk)
endif()

if (ELEMENT_BUILD_TESTS)
    ## Unit tests
    enable_testing()
    add_subdirectory(test)
endif()

if(NOT APPLE)
    # For backward compatibility with the old meson build, not-Mac installs 
    # expect the exe name to be all lower case.
    set(ELEMENT_STANDALONE_PRODUCT_NAME "element")
else()
    set(ELEMENT_STANDALONE_PRODUCT_NAME "Element")
endif()

## Standalone Application
juce_add_gui_app(element_app
    PRODUCT_NAME            "${ELEMENT_STANDALONE_PRODUCT_NAME}"
    BUILD_VERSION           "${ELEMENT_BUILD_NUMBER}"
    DOCUMENT_EXTENSIONS     els elg
    ICON_BIG                data/images/icon.png
    ICON_SMALL              data/images/icon.png

    BUNDLE_ID               "net.kushview.Element"
    MICROPHONE_PERMISSION_ENABLED TRUE

    HARDENED_RUNTIME_ENABLED TRUE
    HARDENED_RUNTIME_OPTIONS
        "com.apple.security.cs.allow-jit"
        "com.apple.security.cs.disable-library-validation"
        "com.apple.security.device.audio-input"
)

target_sources(element_app PRIVATE src/main.cc)
target_link_libraries(element_app PRIVATE kv::element)

if(NOT APPLE)
    install(TARGETS element_app DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(ELEMENT_BUILD_PLUGINS)
    set(ELEMENT_PLUGIN_FORMATS "AU;LV2;VST3")
    if(ELEMENT_ENABLE_VST2)
        list(APPEND ELEMENT_PLUGIN_FORMATS "VST")
    endif()

    ## Instrument Plugin
    juce_add_plugin(element_instrument
        PLUGIN_NAME                 "Element"
        PRODUCT_NAME                "KV-Element"
        BUILD_VERSION               "${ELEMENT_BUILD_NUMBER}"
        PLUGIN_MANUFACTURER_CODE    KshV
        PLUGIN_CODE                 Elmt

        BUNDLE_ID                   "net.kushview.plugins.Element"
        AU_MAIN_TYPE                kAudioUnitType_MusicDevice

        LV2URI                      "https://kushview.net/plugins/element"

        IS_SYNTH                ON
        NEEDS_MIDI_INPUT        ON
        NEEDS_MIDI_OUTPUT       OFF
        EDITOR_WANTS_KEYBOARD_FOCUS ON
        FORMATS                 ${ELEMENT_PLUGIN_FORMATS}

        HARDENED_RUNTIME_ENABLED TRUE
        HARDENED_RUNTIME_OPTIONS
            "com.apple.security.cs.allow-jit"
            "com.apple.security.cs.disable-library-validation"
            "com.apple.security.device.audio-input"
    )
    clap_juce_extensions_plugin(TARGET element_instrument
        CLAP_ID "net.kushview.plugins.Element"
        CLAP_FEATURES instrument)
    target_sources(element_instrument PRIVATE src/plugins/instrument.cc)
    target_link_libraries(element_instrument PRIVATE kv::element)

    if(LINUX)
        install(TARGETS element_instrument_CLAP DESTINATION "${CMAKE_INSTALL_LIBDIR}/clap")
        install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/element_instrument_artefacts/$<CONFIG>/LV2/"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/lv2")
        install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/element_instrument_artefacts/$<CONFIG>/VST3/"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/vst3")
    endif()

    ## Effect Plugin
    juce_add_plugin(element_effect
        PLUGIN_NAME                 "Element FX"
        PRODUCT_NAME                "KV-Element-FX"
        BUILD_VERSION               "${ELEMENT_BUILD_NUMBER}"
        PLUGIN_MANUFACTURER_CODE    KshV
        PLUGIN_CODE                 ElFX

        BUNDLE_ID                   "net.kushview.plugins.ElementFX"
        AU_MAIN_TYPE                kAudioUnitType_Effect

        LV2URI                      "https://kushview.net/plugins/element-fx"
        
        IS_SYNTH                    OFF
        NEEDS_MIDI_INPUT            ON
        NEEDS_MIDI_OUTPUT           OFF
        EDITOR_WANTS_KEYBOARD_FOCUS ON
        FORMATS                     ${ELEMENT_PLUGIN_FORMATS}

        HARDENED_RUNTIME_ENABLED TRUE
        HARDENED_RUNTIME_OPTIONS
            "com.apple.security.cs.allow-jit"
            "com.apple.security.cs.disable-library-validation"
            "com.apple.security.device.audio-input"
    )
    clap_juce_extensions_plugin(TARGET element_effect
        CLAP_ID "net.kushview.ElementFX"
        CLAP_FEATURES audio-effect)

    target_sources(element_effect PRIVATE src/plugins/effect.cc)
    target_link_libraries(element_effect PRIVATE kv::element)

    if(LINUX)
        install(TARGETS element_effect_CLAP DESTINATION "${CMAKE_INSTALL_LIBDIR}/clap")
        install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/element_effect_artefacts/$<CONFIG>/LV2/"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/lv2")
        install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/element_effect_artefacts/$<CONFIG>/VST3/"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/vst3")
    endif()

    if(APPLE)
        ## MIDI Effect Plugin
        juce_add_plugin(element_midi_effect
            PLUGIN_NAME                 "Element MFX"
            PRODUCT_NAME                "KV-Element-MFX"
            BUILD_VERSION               "${ELEMENT_BUILD_NUMBER}"
            PLUGIN_MANUFACTURER_CODE    KshV
            PLUGIN_CODE                 ElMX
            
            BUNDLE_ID                   "net.kushview.plugins.ElementMFX"
            AU_MAIN_TYPE                kAudioUnitType_MIDIProcessor

            IS_SYNTH                    OFF
            NEEDS_MIDI_INPUT            ON
            NEEDS_MIDI_OUTPUT           ON
            EDITOR_WANTS_KEYBOARD_FOCUS ON
            FORMATS                     AU

            HARDENED_RUNTIME_ENABLED TRUE
            HARDENED_RUNTIME_OPTIONS
                "com.apple.security.cs.allow-jit"
                "com.apple.security.cs.disable-library-validation"
                "com.apple.security.device.audio-input"
        )

        target_sources(element_midi_effect PRIVATE src/plugins/midieffect.cc)
        target_link_libraries(element_midi_effect PRIVATE kv::element)
    endif()
endif()

if(LINUX)
    # CPack configuration
    set(CPACK_PACKAGE_NAME "Element")
    set(CPACK_PACKAGE_VENDOR "Kushview")
    set(CPACK_PACKAGE_CONTACT "Kushview Support <support@kushview.net>")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Element - Audio Plugin Host and Modular Instrument")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION_FULL}")
    set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
    set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
    set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Element")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSES/GPL3.txt")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

    set(CPACK_GENERATOR "DEB")
    set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
    set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libasound2;libfreetype6;libx11-6;libxext6;libxrandr2;libxinerama1;libxcursor1;libcurl4;libgl1;libstdc++6")

    # Package file name
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

    include(CPack)
endif()
