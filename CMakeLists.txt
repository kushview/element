# Element CMakeLists.txt

cmake_minimum_required(VERSION 3.22)
project(element VERSION 1.0.0)

if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(deps/sol2 EXCLUDE_FROM_ALL)
add_subdirectory(deps/juce EXCLUDE_FROM_ALL)

cmake_policy(SET CMP0167 OLD)
set(Boost_ROOT "~/SDKs/include")
find_package(Boost REQUIRED)

juce_add_gui_app(element
    COMPANY_NAME            "Kushview"             
    COMPANY_WEBSITE         "https://kushview.net"
    PRODUCT_NAME            "Element"
    DOCUMENT_EXTENSIONS     els elg
    ICON_BIG                data/images/icon.png
    ICON_SMALL              data/images/icon.png
)  

juce_generate_juce_header(element)

target_include_directories(element
    PUBLIC
        include
    PRIVATE
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

target_compile_options(element PRIVATE ${SUIL_CFLAGS_OTHER})
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(element PRIVATE -Wno-sign-conversion -Wno-switch-enum -Wno-attributes)
elseif(MSVC)
    # MSVC doesn't support -Wno-sign-conversion; disable a relevant warning if needed
    # e.g. /wd4244 or /wd4267 depending on the warning you want to suppress
    target_compile_options(element PRIVATE /wd4244)
endif()

target_compile_definitions(element
    PRIVATE
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
    PRIVATE
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
