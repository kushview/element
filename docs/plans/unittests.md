# Unit Test Coverage Analysis

Generated: December 29, 2025

## Overview

This document catalogs the current state of unit test coverage in the Element project, identifying files and classes without unit tests, and provides guidance on using Boost Test for coverage analysis.

## Test Framework

The project uses **Boost.Test** framework for unit testing. Tests are located in the `test/` directory and defined in `test/TestMain.cpp` with the following setup:

```cpp
#define BOOST_TEST_MODULE Element
#include <boost/test/included/unit_test.hpp>
```

## Current Test Coverage

### Existing Unit Tests (33 test suites)

The following test suites are currently implemented:

1. **AlignedDataTests** - Tests for aligned memory allocation
2. **AtomTests** - Tests for atom buffer handling
2. **AtomicValueTests** - Tests for AtomicValue template class
3. **AtomicLockTests** - Tests for AtomicLock class
4. **AudioRoutingTests** - Audio routing functionality
3. **BytesTest** - Byte buffer operations
4. **DataPathTests** - Data path handling
5. **DSPScriptTest** - DSP scripting functionality
6. **Element** - Core element tests
7. **GraphNodeTests** - Graph node operations
8. **IONodeTests** - Input/output node tests
9. **JuceIntegrationTests** - JUCE header compilation tests
10. **LinearFadeTest** - Linear fade engine component
10. **MidiChannelMapTest** - MIDI channel mapping
11. **MidiProgramMapTests** - MIDI program mapping
12. **NodeFactoryTests** - Node factory functionality
13. **NodeObjectTests** - Node object tests
14. **NodeTests** - General node tests
15. **OversamplerTests** - Oversampling functionality
16. **PluginManagerTests** - Plugin management
17. **PortListTests** - Port list operations
18. **PortTypeTests** - Port type handling
19. **RootGraphTests** - Root graph operations
20. **ScriptInfoTest** - Script information
21. **ScriptLoaderTest** - Script loading
22. **ScriptManagerTest** - Script management
23. **ScriptPlayground** - Script experimentation
24. **SessionChangedTest** - Session change detection
25. **ShuttleTests** - Shuttle/transport tests
26. **ToggleGridTest** - Toggle grid engine component
27. **UpdateTests** - Update mechanism tests
28. **VelocityCurveTest** - Velocity curve handling
29. **AtomicValueTests** - AtomicValue template class tests
30. **AtomicLockTests** - AtomicLock synchronization tests

## Files/Classes WITHOUT Unit Tests

### Include Directory (`include/element/`)

The following 51 header files in `include/element/` are **WITHOUT dedicated unit tests**:

#### Core Framework
- [x] `aligneddata.hpp` - Aligned data structures
- [ ] `application.hpp` - Application framework
- [ ] `arc.hpp` - Arc/reference counting (has `arc.cpp` implementation)
- [x] `atomic.hpp` - Atomic operations
- [ ] `audioengine.hpp` - Audio engine interface
- [ ] `context.hpp` - Application context
- [ ] `controller.hpp` - Controller framework
- [ ] `datapipe.hpp` - Data piping
- [ ] `devices.hpp` - Device management
- [ ] `element.h` - C API header
- [ ] `element.hpp` - Main header file
- [ ] `engine.hpp` - Engine interface

#### File System & Data
- [x] `datapath.hpp` - Data path management (has tests: `datapathtests.cpp`)
- [ ] `filesystem.hpp` - File system utilities
- [x] `gzip.hpp` - Gzip compression

#### JUCE Integration
- [x] `juce.hpp` - JUCE framework integration
- [x] `juce/` - JUCE-specific headers subdirectory

#### Data Structures & Utilities
- [x] `atombuffer.hpp` - Atom buffer (has tests: `atomtests.cpp`)
- [ ] `linkedlist.hpp` - Linked list implementation
- [ ] `symbolmap.hpp` - Symbol mapping
- [ ] `timescale.hpp` - Time scale operations
- [ ] `transport.hpp` - Transport control

#### MIDI
- [ ] `midichannels.hpp` - MIDI channel utilities
- [ ] `midiiomonitor.hpp` - MIDI I/O monitoring
- [ ] `midipipe.hpp` - MIDI pipe operations

#### Node System
- [ ] `node.h` - Node C API
- [x] `node.hpp` - Node class (has tests: `NodeTests.cpp`)
- [x] `nodefactory.hpp` - Node factory (has tests: `NodeFactoryTests.cpp`)
- [ ] `nodeproxy.hpp` - Node proxy pattern

#### Parameters & Ports
- [ ] `parameter.hpp` - Parameter handling
- [ ] `portcount.hpp` - Port counting
- [x] `porttype.hpp` - Port type definitions (has tests: `porttypetests.cpp`)

#### Plugin System
- [ ] `plugins.hpp` - Plugin interfaces
- [x] `oversampler.hpp` - Oversampling (has tests: `OversamplerTests.cpp`)
- [ ] `processor.hpp` - Processor base class

#### Presets & Sessions
- [ ] `presets.hpp` - Preset management
- [ ] `session.hpp` - Session handling

#### Scripting
- [ ] `lua.hpp` - Lua integration
- [ ] `script.hpp` - Script interface

#### Services & Settings
- [ ] `services.hpp` - Service layer
- [ ] `settings.hpp` - Settings management

#### Signals & State
- [ ] `signals.hpp` - Signal/slot mechanism
- [ ] `runmode.hpp` - Run mode enumeration
- [x] `shuttle.hpp` - Shuttle control (has tests: `shuttletests.cpp`)

#### UI
- [ ] `ui.hpp` - UI framework
- [ ] `ui/` - UI components subdirectory

#### Graph & Model
- [x] `graph.hpp` - Graph structure (has tests: `GraphNodeTests.cpp`, `RootGraphTests.cpp`)
- [ ] `model.hpp` - Data model
- [ ] `tags.hpp` - Tag system

#### Version
- [ ] `version.h.in` - Version template
- [ ] `version.hpp` - Version information

### Source Directory (`src/`)

The project has **369 source files** (.cpp and .hpp) in the `src/` directory. Major categories without comprehensive test coverage include:

#### Services (`src/services/`) - **NO TESTS**
- `deviceservice.cpp/hpp` - Device service
- `engineservice.cpp` - Engine service (33KB, core functionality)
- `guiservice.cpp` - GUI service (39KB, large UI service)
- `mappingservice.cpp/hpp` - Mapping service
- `oscservice.cpp/hpp` - OSC service
- `presetservice.cpp/hpp` - Preset service
- `sessionservice.cpp/hpp` - Session service

#### UI Components (`src/ui/`) - **NO TESTS** (~70+ files)
All UI component files lack unit tests, including:
- `mainwindow.cpp` - Main application window
- `grapheditorview.cpp/hpp` - Graph editor view
- `nodeproperties.cpp` - Node properties editor
- `audiodeviceselector.cpp` - Audio device configuration
- `pluginwindow.cpp` - Plugin window management
- `sessiontreeview.hpp` - Session tree view
- `scripteditorview.hpp` - Script editor
- `luaconsole.cpp/hpp` - Lua console
- And many more UI components...

#### Core Implementation (`src/`)
Files without dedicated tests:
- `application.cpp` - Application main class
- `arc.cpp` - Arc implementation (reference counting)
- `bindings.cpp` - Language bindings
- `commands.cpp` - Command handling
- `context.cpp` - Context implementation
- `controller.cpp` - Controller implementation
- `devicemanager.cpp` - Device manager
- `filesystemwatcher.cpp/hpp` - File system monitoring
- `graph.cpp` - Graph implementation
- `matrixstate.cpp/hpp` - Matrix state
- `messages.cpp/hpp` - Message passing
- `model.cpp` - Model implementation
- `module.cpp/hpp` - Module system
- `node.cpp` - Node implementation
- `plugineditor.cpp/hpp` - Plugin editor
- `pluginmanager.cpp` - Plugin manager
- `pluginprocessor.cpp/hpp` - Plugin processor
- `presetmanager.hpp` - Preset manager
- `ringbuffer.cpp/hpp` - Ring buffer
- `script.cpp` - Script implementation
- `scripting.cpp/hpp` - Scripting system
- `semaphore.cpp/hpp` - Semaphore implementation
- `services.cpp` - Services implementation
- `session.cpp` - Session implementation
- `settings.cpp` - Settings implementation
- `strings.cpp` - String utilities
- `tempo.hpp` - Tempo handling
- `timescale.cpp` - Time scale operations
- `utils.cpp/hpp` - Utility functions

#### Engine Components (`src/engine/`) - **PARTIAL COVERAGE**
Files with NO tests:
- `audioengine.cpp` - Core audio engine
- `clapprovider.cpp/hpp` - CLAP plugin provider
- `graphbuilder.cpp/hpp` - Graph building
- `graphmanager.cpp/hpp` - Graph management
- `graphnode.cpp/hpp` - Graph nodes (partial - has `GraphNodeTests.cpp`)
- `internalformat.cpp/hpp` - Internal audio format
- `jack.cpp/hpp` - JACK audio backend
- `mappingengine.cpp/hpp` - Parameter mapping engine
- `midiclock.cpp/hpp` - MIDI clock
- `midiengine.cpp/hpp` - MIDI engine
- `midipanic.hpp` - MIDI panic
- `midipipe.cpp` - MIDI pipe
- `miditranspose.hpp` - MIDI transposition
- `nodefactory.cpp` - Node factory implementation
- `oversampler.cpp` - Oversampler (has `OversamplerTests.cpp`)
- `parameter.cpp` - Parameter implementation
- `portbuffer.cpp/hpp` - Port buffer
- `processor.cpp` - Processor implementation
- `rootgraph.cpp/hpp` - Root graph (has `RootGraphTests.cpp`)
- `shuttle.cpp` - Shuttle implementation (has `shuttletests.cpp`)
- `transport.cpp` - Transport implementation
- `trace.hpp` - Tracing utilities

#### Node Implementations (`src/nodes/`) - **NO TESTS** (~60+ node types)
All node type implementations lack dedicated tests:
- `audiofileplayer.cpp/hpp` - Audio file player
- `audiomixer.cpp/hpp` - Audio mixer
- `audioprocessor.hpp` - Audio processor base
- `audiorouter.cpp/hpp` - Audio router (has `audioroutingtests.cpp`)
- `compressor.cpp/hpp` - Compressor
- `eqfilter.cpp/hpp` - EQ filter
- `mediaplayer.cpp/hpp` - Media player
- `midichannelsplitter.cpp/hpp` - MIDI channel splitter
- `mididevice.cpp/hpp` - MIDI device
- `midifilter.cpp/hpp` - MIDI filter
- `midimonitor.cpp/hpp` - MIDI monitor
- `midiprogrammap.cpp/hpp` - MIDI program map (has `MidiProgramMapTests.cpp`)
- `midirouter.cpp/hpp` - MIDI router
- `midisetlist.cpp/hpp` - MIDI setlist
- `oscreceiver.cpp/hpp` - OSC receiver
- `oscsender.cpp/hpp` - OSC sender
- `scriptnode.cpp/hpp` - Script node
- `volume.hpp` - Volume control
- And many more node types...

#### Scripting System (`src/scripting/`) - **PARTIAL COVERAGE**
- `bindings.cpp/hpp` - Language bindings
- `dspscript.cpp/hpp` - DSP script (has `dspscripttest.cpp`)
- `dspuiscript.cpp/hpp` - DSP UI script
- `scriptinstance.hpp` - Script instance
- `scriptloader.cpp/hpp` - Script loader (has `scriptloadertest.cpp`)
- `scriptmanager.cpp/hpp` - Script manager (has `scriptmanagertest.cpp`)
- `scriptsource.hpp` - Script source

#### Lua Bindings (`src/el/`) - **NO TESTS** (~30+ files)
All Lua binding files for the element API:
- `AudioBuffer32.cpp/64.cpp` - Audio buffer bindings
- `Bounds.cpp` - Bounds bindings
- `Commands.cpp` - Command bindings
- `Content.cpp` - Content bindings
- `Context.cpp` - Context bindings
- `Desktop.cpp` - Desktop bindings
- `Graph.cpp` - Graph bindings
- `GraphEditor.cpp` - Graph editor bindings
- `Graphics.cpp` - Graphics bindings
- `MidiBuffer.cpp` - MIDI buffer bindings
- `MidiMessage.cpp` - MIDI message bindings
- `MouseEvent.cpp` - Mouse event bindings
- `Node.cpp` - Node bindings
- `Parameter.cpp` - Parameter bindings
- `Point.cpp` - Point bindings
- `Range.cpp` - Range bindings
- `Rectangle.cpp` - Rectangle bindings
- `Session.cpp` - Session bindings
- `Slider.cpp` - Slider bindings
- `TextButton.cpp` - Text button bindings
- `View.cpp` - View bindings
- `Widget.cpp` - Widget bindings

#### LV2 Support (`src/lv2/`) - **NO TESTS**
- `messages.cpp/hpp` - LV2 message handling

## Using Boost.Test for Coverage Analysis

### Overview

Boost.Test itself does **not provide built-in code coverage analysis**. However, you can combine Boost.Test with standard code coverage tools to measure test coverage.

### Recommended Coverage Tools

#### 1. **gcov/lcov (GCC/Clang)**

The most common approach on Linux/macOS:

**Step 1: Compile with coverage flags**

Modify your CMake configuration or add flags:

```cmake
# Add to CMakeLists.txt or use cmake flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
```

Or configure from command line:
```bash
cmake -DCMAKE_CXX_FLAGS="--coverage" -DCMAKE_EXE_LINKER_FLAGS="--coverage" ..
```

**Step 2: Build and run tests**
```bash
cmake --build . --target test_element
./test_element
```

**Step 3: Generate coverage reports**

Using gcov:
```bash
# Generate coverage data for individual files
gcov src/*.cpp
```

Using lcov (HTML reports):
```bash
# Capture coverage data
lcov --capture --directory . --output-file coverage.info

# Filter out system/library headers
lcov --remove coverage.info '/usr/*' '*/deps/*' '*/test/*' --output-file coverage_filtered.info

# Generate HTML report
genhtml coverage_filtered.info --output-directory coverage_html

# View in browser
open coverage_html/index.html
```

**Step 4: Clean coverage data between runs**
```bash
lcov --zerocounters --directory .
rm -f *.gcda *.gcno
```

#### 2. **llvm-cov (Clang/LLVM)**

For projects using Clang:

```bash
# Compile with coverage
cmake -DCMAKE_CXX_FLAGS="-fprofile-instr-generate -fcoverage-mapping" ..
cmake --build .

# Run tests to generate profile data
LLVM_PROFILE_FILE="test_element.profraw" ./test_element

# Convert profile data
llvm-profdata merge -sparse test_element.profraw -o test_element.profdata

# Generate coverage report
llvm-cov show ./test_element -instr-profile=test_element.profdata -format=html -output-dir=coverage_html

# Or text summary
llvm-cov report ./test_element -instr-profile=test_element.profdata
```

#### 3. **gcovr (Cross-platform)**

A Python tool that works with gcov but provides better output:

```bash
# Install gcovr
pip install gcovr

# After running tests with --coverage flags
gcovr --root . --exclude '.*test.*' --exclude '.*deps.*' --html --html-details -o coverage.html

# Or XML for CI/CD
gcovr --root . --exclude '.*test.*' --xml -o coverage.xml
```

### Integration with CMake/CTest

Add coverage target to `test/CMakeLists.txt`:

```cmake
# Add coverage option
option(ENABLE_COVERAGE "Enable code coverage analysis" OFF)

if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(test_element PRIVATE --coverage)
        target_link_options(test_element PRIVATE --coverage)
        
        # Add custom target for coverage report
        find_program(LCOV lcov)
        find_program(GENHTML genhtml)
        
        if(LCOV AND GENHTML)
            add_custom_target(coverage
                COMMAND ${CMAKE_COMMAND} -E remove_directory coverage_html
                COMMAND ${CMAKE_COMMAND} -E make_directory coverage_html
                COMMAND lcov --zerocounters --directory .
                COMMAND $<TARGET_FILE:test_element>
                COMMAND lcov --capture --directory . --output-file coverage.info
                COMMAND lcov --remove coverage.info '/usr/*' '*/deps/*' '*/test/*' --output-file coverage_filtered.info
                COMMAND genhtml coverage_filtered.info --output-directory coverage_html
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating code coverage report"
            )
        endif()
    endif()
endif()
```

Usage:
```bash
cmake -DENABLE_COVERAGE=ON ..
cmake --build .
cmake --build . --target coverage
```

### Running Specific Test Suites

Boost.Test allows running specific test suites for targeted coverage:

```bash
# Run all tests
./test_element

# Run specific test suite
./test_element --run_test=NodeTests

# Run specific test case
./test_element --run_test=NodeTests/specific_test_case

# List all tests
./test_element --list_content

# Verbose output
./test_element --log_level=all
```

### CI/CD Integration

For continuous integration, you can:

1. Generate XML coverage reports
2. Upload to coverage services (Codecov, Coveralls)
3. Set coverage thresholds

Example GitHub Actions workflow:
```yaml
- name: Configure with coverage
  run: cmake -DENABLE_COVERAGE=ON -B build

- name: Build
  run: cmake --build build

- name: Run tests
  run: cd build && ctest --output-on-failure

- name: Generate coverage
  run: |
    lcov --capture --directory build --output-file coverage.info
    lcov --remove coverage.info '/usr/*' '*/deps/*' --output-file coverage.info
    
- name: Upload to Codecov
  uses: codecov/codecov-action@v3
  with:
    files: ./coverage.info
```

## Summary Statistics

- **Total Header Files (include/element/)**: ~51 files
- **Total Source Files (src/)**: 369 files (.cpp and .hpp)
- **Test Suites Implemented**: 31 suites
- **Test Files**: ~30 test .cpp files
- **Estimated Coverage**: ~15-20% (based on file counts)

### Recent Additions
- **December 30, 2025**: Added comprehensive tests for `atomic.hpp` (AtomicValue and AtomicLock)
- **December 30, 2025**: Added JUCE integration compilation tests (`juce.hpp` and `juce/` headers)

## Priority Areas for New Tests

Based on size, complexity, and criticality, recommended priorities for new unit tests:

### High Priority
1. **Services** - Core application services (0% coverage)
   - `engineservice.cpp` (33KB)
   - `guiservice.cpp` (39KB)
   - `sessionservice.cpp`
   - `mappingservice.cpp`

2. **Core Engine** - Audio processing core
   - `audioengine.cpp`
   - `graphmanager.cpp/hpp`
   - `midiengine.cpp`
   - `transport.cpp`

3. **Plugin System**
   - `pluginmanager.cpp`
   - `pluginprocessor.cpp`
   - `clapprovider.cpp`

### Medium Priority
4. **Session & Model**
   - `session.cpp`
   - `model.cpp`
   - `context.cpp`

5. **Key Node Types**
   - `audiomixer.cpp`
   - `audiorouter.cpp` (partial coverage exists)
   - `compressor.cpp`
   - `eqfilter.cpp`

6. **Scripting**
   - Lua bindings (`src/el/*.cpp`)
   - `scriptnode.cpp`

### Lower Priority (UI Testing)
7. **UI Components** - Consider integration/E2E tests instead
   - Main window
   - Graph editor
   - Plugin windows

## Recommendations

1. **Start with Core Services**: The services layer is critical and completely untested.

2. **Use Test Fixtures**: Create shared fixtures for common test setups (see `TestMain.cpp` for the `JuceMessageManagerFixture` example).

3. **Focus on Logic, Not UI**: UI components are harder to unit test. Focus on business logic and engine components first.

4. **Leverage Coverage Tools**: Implement lcov/gcov integration to track progress and identify untested code paths.

5. **Add Tests Incrementally**: When fixing bugs or adding features, add corresponding unit tests.

6. **Consider Integration Tests**: For complex subsystems (services, engine), integration tests may be more valuable than isolated unit tests.

7. **Mock External Dependencies**: Use mocking for file system, device managers, and plugin scanning in tests.

## Notes

- The project already uses a good test structure with Boost.Test
- CTest integration is properly configured
- Most tests focus on core data structures and algorithms (correct approach)
- UI layer is intentionally untested (common and reasonable)
- Engine components have partial coverage
- The dependency subdirectories (deps/) have their own tests

---

*This document should be updated as new tests are added or when coverage analysis is performed.*
