# Element CMakeLists.txt
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.22)
project(element VERSION 1.0.0)

include(cmake/Element.cmake)

add_subdirectory(deps/sol2 EXCLUDE_FROM_ALL)

# VST2 SDK setup
if(ELEMENT_USE_VST2)
    set(JUCE_GLOBAL_VST2_SDK_PATH "${USER_HOME_DIRECTORY}/SDKs/vstsdk2.4")
endif()
add_subdirectory(deps/juce EXCLUDE_FROM_ALL)

# CLAP Setup
add_subdirectory(deps/clap EXCLUDE_FROM_ALL)
add_subdirectory(deps/clap-helpers EXCLUDE_FROM_ALL)
# add_subdirectory(deps/clap-juce-extensions EXCLUDE_FROM_ALL)

# BOOST Setup
cmake_policy(SET CMP0167 OLD)
set(Boost_ROOT "${USER_HOME_DIRECTORY}/SDKs/include")
find_package(Boost REQUIRED)

# Element static library
add_library(element STATIC)
add_library(kv::element ALIAS element)

target_include_directories(element
    PUBLIC
        include
    PUBLIC
        deps/clap/include
        deps/clap-helpers/include
        deps/lvtk/include
        src
        src/lua/sol
        src/lua/src
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
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)

# VST2 enable host support
if(ELEMENT_USE_VST2)
    target_compile_definitions(element PUBLIC JUCE_PLUGINHOST_VST=1)
    target_link_libraries(element PUBLIC juce_vst2_sdk)
endif()

## Standalone Application
juce_add_gui_app(element_app
    COMPANY_NAME            "Kushview"             
    COMPANY_WEBSITE         "https://kushview.net"
    PRODUCT_NAME            "Element"
    DOCUMENT_EXTENSIONS     els elg
    ICON_BIG                data/images/icon.png
    ICON_SMALL              data/images/icon.png
)

target_sources(element_app PRIVATE src/main.cc)
target_link_libraries(element_app PRIVATE kv::element)

set(ELEMENT_PLUGIN_FORMATS "AU;LV2;VST3")
if(ELEMENT_USE_VST2)
    list(APPEND ELEMENT_PLUGIN_FORMATS "VST")
endif()

## Instrument Plugin
juce_add_plugin(element_instrument
    COMPANY_NAME            "Kushview"
    COMPANY_WEBSITE         "https://kushview.net"
    PLUGIN_NAME             "Element"
    PRODUCT_NAME            "KV-Element"
    PLUGIN_CODE             Elmt
    IS_SYNTH                OFF
    NEEDS_MIDI_INPUT        ON
    NEEDS_MIDI_OUTPUT       OFF
    EDITOR_WANTS_KEYBOARD_FOCUS ON
    FORMATS                 ${ELEMENT_PLUGIN_FORMATS}
)

target_sources(element_instrument PRIVATE plugins/instrument/plugin.cpp)
target_link_libraries(element_instrument PRIVATE kv::element)

## Effect Plugin
juce_add_plugin(element_effect
    COMPANY_NAME            "Kushview"
    COMPANY_WEBSITE         "https://kushview.net"
    PLUGIN_NAME             "Element Effect"
    PRODUCT_NAME            "KV-Element-FX"
    PLUGIN_CODE             ElFX
    IS_SYNTH                OFF
    NEEDS_MIDI_INPUT        ON
    NEEDS_MIDI_OUTPUT       OFF
    EDITOR_WANTS_KEYBOARD_FOCUS ON
    FORMATS                 ${ELEMENT_PLUGIN_FORMATS}
)

target_sources(element_effect PRIVATE plugins/effect/plugin.cpp)
target_link_libraries(element_effect PRIVATE kv::element)

## MIDI Effect Plugin
juce_add_plugin(element_midi_effect
    COMPANY_NAME            "Kushview"
    COMPANY_WEBSITE         "https://kushview.net"
    PLUGIN_NAME             "Element MIDI Effect"
    PRODUCT_NAME            "KV-Element-MFX"
    PLUGIN_CODE             ElMX
    IS_SYNTH                OFF
    NEEDS_MIDI_INPUT        ON
    NEEDS_MIDI_OUTPUT       ON
    EDITOR_WANTS_KEYBOARD_FOCUS ON
    FORMATS                 ${ELEMENT_PLUGIN_FORMATS}
)

target_sources(element_midi_effect PRIVATE plugins/midieffect/plugin.cpp)
target_link_libraries(element_midi_effect PRIVATE kv::element)
