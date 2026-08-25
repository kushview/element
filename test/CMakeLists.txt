# SPDX-FileCopyrightText: 2026 Kushview, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

file(GLOB_RECURSE TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

juce_add_console_app(test_element)
target_sources(test_element PRIVATE ${TEST_SOURCES})
target_link_libraries(test_element PRIVATE kv::element)
target_include_directories(test_element
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        ${GENERATED_INCLUDE_DIR}
        ${Boost_INCLUDE_DIRS})
target_compile_definitions(test_element
    PRIVATE
        EL_TEST_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
        EL_TEST_SCANNER_EXE_PATH="$<TARGET_FILE:element_app>")

# Deliberately misbehaving "plugins" for PluginScannerTests: modules that
# crash or hang on load, laid out as VST3s so the scanner worker loads them.
# Opt-in at runtime via EL_TEST_SCANNER_INTEGRATION=1. The $<1:...> generator
# expressions stop multi-config generators appending a per-config directory.
foreach(_kind IN ITEMS crasher hanger)
    add_library(el_test_${_kind} MODULE fixture/badplugin.c)
    set(_dir "${CMAKE_CURRENT_BINARY_DIR}/badplugins/${_kind}")
    if(WIN32)
        # Single-file VST3: a renamed DLL.
        set_target_properties(el_test_${_kind} PROPERTIES
            PREFIX "" OUTPUT_NAME "${_kind}" SUFFIX ".vst3"
            LIBRARY_OUTPUT_DIRECTORY "$<1:${_dir}>")
    elseif(APPLE)
        set(_contents "${_dir}/${_kind}.vst3/Contents")
        set_target_properties(el_test_${_kind} PROPERTIES
            PREFIX "" OUTPUT_NAME "${_kind}" SUFFIX ""
            LIBRARY_OUTPUT_DIRECTORY "$<1:${_contents}/MacOS>")
        file(WRITE "${_contents}/Info.plist"
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">
<plist version=\"1.0\">
<dict>
    <key>CFBundleExecutable</key><string>${_kind}</string>
    <key>CFBundleIdentifier</key><string>net.kushview.test.${_kind}</string>
    <key>CFBundlePackageType</key><string>BNDL</string>
</dict>
</plist>
")
    else()
        set_target_properties(el_test_${_kind} PROPERTIES
            PREFIX "" OUTPUT_NAME "${_kind}" SUFFIX ".so"
            LIBRARY_OUTPUT_DIRECTORY
                "$<1:${_dir}/${_kind}.vst3/Contents/${CMAKE_SYSTEM_PROCESSOR}-linux>")
    endif()
    add_dependencies(test_element el_test_${_kind})
endforeach()
target_compile_definitions(el_test_hanger PRIVATE EL_TEST_HANG=1)
target_compile_definitions(test_element PRIVATE
    EL_TEST_BADPLUGINS_DIR="${CMAKE_CURRENT_BINARY_DIR}/badplugins")

# Register all test suites with CTest
add_test(NAME "AtomicValueTests" COMMAND test_element --run_test=AtomicValueTests)
add_test(NAME "AtomicLockTests" COMMAND test_element --run_test=AtomicLockTests)
add_test(NAME "AudioRoutingTests" COMMAND test_element --run_test=AudioRoutingTests)
add_test(NAME "AuthTests" COMMAND test_element --run_test=AuthTests)
add_test(NAME "BytesTest" COMMAND test_element --run_test=BytesTest)
add_test(NAME "JuceIntegrationTests" COMMAND test_element --run_test=JuceIntegrationTests)
add_test(NAME "DataPathTests" COMMAND test_element --run_test=DataPathTests)
add_test(NAME "DeviceMonitorTests" COMMAND test_element --run_test=DeviceMonitorTests)
add_test(NAME "DSPScriptTest" COMMAND test_element --run_test=DSPScriptTest)
add_test(NAME "Element" COMMAND test_element --run_test=Element)
add_test(NAME "GraphNodeTests" COMMAND test_element --run_test=GraphNodeTests)
add_test(NAME "GzipTests" COMMAND test_element --run_test=GzipTests)
add_test(NAME "IONodeTests" COMMAND test_element --run_test=IONodeTests)
add_test(NAME "LinearFadeTest" COMMAND test_element --run_test=LinearFadeTest)
add_test(NAME "MidiChannelMapTest" COMMAND test_element --run_test=MidiChannelMapTest)
add_test(NAME "MidiMappingModelTests" COMMAND test_element --run_test=MidiMappingModelTests)
add_test(NAME "MappingTargetTests" COMMAND test_element --run_test=MappingTargetTests)
add_test(NAME "MidiMappingSessionTests" COMMAND test_element --run_test=MidiMappingSessionTests)
add_test(NAME "MappingEngineTests" COMMAND test_element --run_test=MappingEngineTests)
add_test(NAME "MappingLearnTests" COMMAND test_element --run_test=MappingLearnTests)
add_test(NAME "MidiClockTest" COMMAND test_element --run_test=MidiClockTest)
add_test(NAME "MidiMonitorTests" COMMAND test_element --run_test=MidiMonitorTests)
add_test(NAME "MidiProgramMapTests" COMMAND test_element --run_test=MidiProgramMapTests)
add_test(NAME "MidiProgramTests" COMMAND test_element --run_test=MidiProgramTests)
add_test(NAME "MidiScriptTests" COMMAND test_element --run_test=MidiScriptTests)
if (APPLE)
    add_test(NAME "MidiOutputTests" COMMAND test_element --run_test=MidiOutputTests)
endif()
add_test(NAME "NodeFactoryTests" COMMAND test_element --run_test=NodeFactoryTests)
add_test(NAME "NodeObjectTests" COMMAND test_element --run_test=NodeObjectTests)
add_test(NAME "NodeTests" COMMAND test_element --run_test=NodeTests)
add_test(NAME "OversamplerTests" COMMAND test_element --run_test=OversamplerTests)
add_test(NAME "PluginManagerTests" COMMAND test_element --run_test=PluginManagerTests)
add_test(NAME "PluginMetadataTests" COMMAND test_element --run_test=PluginMetadataTests)
add_test(NAME "PluginScannerTests" COMMAND test_element --run_test=PluginScannerTests)
add_test(NAME "PortListTests" COMMAND test_element --run_test=PortListTests)
add_test(NAME "PresetScriptsTest" COMMAND test_element --run_test=PresetScriptsTest)
add_test(NAME "PortTypeTests" COMMAND test_element --run_test=PortTypeTests)
add_test(NAME "RootGraphTests" COMMAND test_element --run_test=RootGraphTests)
add_test(NAME "ScriptInfoTest" COMMAND test_element --run_test=ScriptInfoTest)
add_test(NAME "ScriptLoaderTest" COMMAND test_element --run_test=ScriptLoaderTest)
add_test(NAME "ScriptManagerTest" COMMAND test_element --run_test=ScriptManagerTest)
add_test(NAME "ScriptPlayground" COMMAND test_element --run_test=ScriptPlayground)
add_test(NAME "SessionChangedTest" COMMAND test_element --run_test=SessionChangedTest)
add_test(NAME "SessionLoadBenchTests" COMMAND test_element --run_test=SessionLoadBenchTests)
add_test(NAME "ShuttleTests" COMMAND test_element --run_test=ShuttleTests)
add_test(NAME "TapTempoTests" COMMAND test_element --run_test=TapTempoTests)
add_test(NAME "TransportTests" COMMAND test_element --run_test=TransportTests)
add_test(NAME "ToggleGridTest" COMMAND test_element --run_test=ToggleGridTest)
add_test(NAME "UpdateTests" COMMAND test_element --run_test=UpdateTests)
add_test(NAME "VelocityCurveTest" COMMAND test_element --run_test=VelocityCurveTest)
