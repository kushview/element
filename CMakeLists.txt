# Element CMakeLists.txt
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.22)
project(element VERSION 1.0.0)

include(cmake/Element.cmake)

add_subdirectory(deps/sol2 EXCLUDE_FROM_ALL)

# JUCE VST2 SDK setup
if(ELEMENT_ENABLE_VST2)
    set(JUCE_GLOBAL_VST2_SDK_PATH "${USER_HOME_DIRECTORY}/SDKs/vstsdk2.4")
endif()
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

find_package(Boost 1.80.0)
if(NOT Boost_FOUND)
    message(" -- Checking user SDK's for boost")
    set(Boost_ROOT "${USER_HOME_DIRECTORY}/SDKs/include")
    find_package(Boost 1.80.0 REQUIRED)
endif()

# Element static library
add_library(element STATIC)
add_library(kv::element ALIAS element)

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

file(GLOB_RECURSE ELEMENT_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

target_sources(element PRIVATE ${ELEMENT_SOURCES})

if(APPLE)
    target_sources(element PRIVATE src/ui/nsviewwithparent.mm)
endif()

target_compile_definitions(element PUBLIC BOOST_ALL_NO_LIB=1)

target_compile_options(element PRIVATE ${SUIL_CFLAGS_OTHER})
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(element PRIVATE -Wno-sign-conversion -Wno-switch-enum -Wno-attributes)
elseif(MSVC)
    # MSVC doesn't support -Wno-sign-conversion; disable a relevant warning if needed
    # e.g. /wd4244 or /wd4267 depending on the warning you want to suppress
    target_compile_options(element PRIVATE /wd4244)
endif()

target_compile_definitions(element
    PUBLIC
        ELEMENT_CMAKE=1
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

## Standalone Application
juce_add_gui_app(element_app
    PRODUCT_NAME            "Element"
    DOCUMENT_EXTENSIONS     els elg
    ICON_BIG                data/images/icon.png
    ICON_SMALL              data/images/icon.png

    BUNDLE_ID               "net.kushview.Element"
    MICROPHONE_PERMISSION_ENABLED TRUE
)

target_sources(element_app PRIVATE src/main.cc)
target_link_libraries(element_app PRIVATE kv::element)

if(ELEMENT_BUILD_PLUGINS)
    set(ELEMENT_PLUGIN_FORMATS "AU;LV2;VST3")
    if(ELEMENT_ENABLE_VST2)
        list(APPEND ELEMENT_PLUGIN_FORMATS "VST")
    endif()

    ## Instrument Plugin
    juce_add_plugin(element_instrument
        PLUGIN_NAME                 "Element"
        PRODUCT_NAME                "KV-Element"
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
    )
    clap_juce_extensions_plugin(TARGET element_instrument
        CLAP_ID "net.kushview.plugins.Element"
        CLAP_FEATURES instrument)
    target_sources(element_instrument PRIVATE plugins/instrument/plugin.cpp)
    target_link_libraries(element_instrument PRIVATE kv::element)

    ## Effect Plugin
    juce_add_plugin(element_effect
        PLUGIN_NAME                 "Element Effect"
        PRODUCT_NAME                "KV-Element-FX"
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
    )
    clap_juce_extensions_plugin(TARGET element_effect
        CLAP_ID "net.kushview.ElementFX"
        CLAP_FEATURES audio-effect)

    target_sources(element_effect PRIVATE plugins/effect/plugin.cpp)
    target_link_libraries(element_effect PRIVATE kv::element)

    ## MIDI Effect Plugin
    juce_add_plugin(element_midi_effect
        COMPANY_NAME                "Kushview"
        COMPANY_WEBSITE             "https://kushview.net"
        PLUGIN_NAME                 "Element MIDI Effect"
        PRODUCT_NAME                "KV-Element-MFX"
        PLUGIN_MANUFACTURER_CODE    KshV
        PLUGIN_CODE                 ElMX
        
        BUNDLE_ID                   "net.kushview.plugins.ElementMFX"
        AU_MAIN_TYPE                kAudioUnitType_MIDIProcessor

        IS_SYNTH                    OFF
        NEEDS_MIDI_INPUT            ON
        NEEDS_MIDI_OUTPUT           ON
        EDITOR_WANTS_KEYBOARD_FOCUS ON
        FORMATS                     AU
    )

    target_sources(element_midi_effect PRIVATE plugins/midieffect/plugin.cpp)
    target_link_libraries(element_midi_effect PRIVATE kv::element)
endif()
