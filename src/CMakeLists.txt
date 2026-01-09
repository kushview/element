# Element static library
add_library(element STATIC)
add_library(kv::element ALIAS element)

set_target_properties(element PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(element
    PUBLIC
        "${GENERATED_INCLUDE_DIR}"
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/src"
        "${PROJECT_SOURCE_DIR}/src/lua/sol"
        "${PROJECT_SOURCE_DIR}/src/lua/src"
        "${PROJECT_SOURCE_DIR}/deps/clap-juce-extensions/clap-libs/clap/include"
        "${PROJECT_SOURCE_DIR}/deps/clap-juce-extensions/clap-libs/clap-helpers/include"
        "${PROJECT_SOURCE_DIR}/deps/lvtk/include"
        "${Boost_INCLUDE_DIRS}"       
)

file(GLOB_RECURSE ELEMENT_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

target_sources(element PRIVATE ${ELEMENT_SOURCES})

if(APPLE)
    target_sources(element PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/ui/nsviewwithparent.mm")
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
        JUCE_USE_XRENDER=1
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

if(ELEMENT_ENABLE_UPDATER)
    message(STATUS "Element updater enabled")
    target_compile_definitions(element PUBLIC ELEMENT_UPDATER=1)
endif()

if(ELEMENT_ENABLE_LTO)
    message("Link Time Optimization enabled")
    target_link_libraries(element PUBLIC juce::juce_recommended_lto_flags)
endif()

if(WIN32 AND ELEMENT_ENABLE_ASIO)
    message("ASIO audio device support enabled")
    target_include_directories(element PUBLIC "${ASIOSDK_SOURCE_DIR}/common")
    target_compile_definitions(element PUBLIC JUCE_ASIO=1)
endif()

# VST2 enable host support
if(ELEMENT_ENABLE_VST2)
    message(STATUS "VST2 Plugin support enabled")
    target_compile_definitions(element PUBLIC JUCE_PLUGINHOST_VST=1)
    target_link_libraries(element PUBLIC juce_vst2_sdk)
endif()
