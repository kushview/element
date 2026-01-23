# AudioUnit Symbol Visibility Issue - Analysis Report

**Date**: January 22, 2026  
**Issue**: Element standalone app hangs when scanning its own AudioUnit plugin; AU crashes with memory corruption when loaded directly

## Problem Statement

### Primary Issue
Element standalone application hangs indefinitely when scanning its own AudioUnit plugin during out-of-process plugin scanning. Other formats (VST3, LV2, CLAP) scan successfully.

### Secondary Issue
When loading Element AU directly (unverified), it crashes with:
```
malloc: *** error for object 0x600000f64b70: pointer being freed was not allocated
```

Stack trace shows crash in `juce::AudioChannelSet::getSpeakerArrangementAsString()`, indicating memory corruption in JUCE's static data.

## Key Observations

1. **Format-specific behavior**: Only AudioUnit hangs/crashes; VST3, LV2, and CLAP work correctly
2. **External validation works**: pluginval successfully validates Element AU without issues
3. **Shared static library**: Both standalone app and plugins link the same `src/libelement.a` containing JUCE code
4. **PluginProcessor initialization**: Not the root cause - plugins never run as standalone, and other formats don't need special handling

## Technical Analysis

### Build Configuration Issues

#### Missing Symbol Visibility Flags

**Problem**: Neither the `element` static library nor the AudioUnit plugin targets have symbol visibility controls enabled.

**Current state** (from compile_commands.json):
```bash
# Plugin compilation:
-fvisibility-inlines-hidden  # Only hides inline functions
# Missing: -fvisibility=hidden
```

**CMake configuration** (src/CMakeLists.txt):
```cmake
add_library(element STATIC ...)
set_target_properties(element PROPERTIES
    POSITION_INDEPENDENT_CODE ON  # Present
    # Missing: CXX_VISIBILITY_PRESET hidden
    # Missing: C_VISIBILITY_PRESET hidden
    # Missing: VISIBILITY_INLINES_HIDDEN ON
)
```

**Plugin targets** (main CMakeLists.txt):
```cmake
juce_add_plugin(element_instrument_AU ...)
# Missing: CXX_VISIBILITY_PRESET hidden
# Missing: C_VISIBILITY_PRESET hidden
# Missing: VISIBILITY_INLINES_HIDDEN ON
```

#### Comparison with Working CLAP Format

CLAP plugins work correctly because `clap-juce-extensions` properly sets:
```cmake
set_target_properties(${target} PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
```

### Symbol Collision Analysis

#### Exported Symbols
Using `nm -g` on standalone binary:
```bash
$ nm -g build/element_app_artefacts/Debug/Element.app/.../Element | grep "juce.*AudioChannelSet"
# Shows many exported JUCE symbols
```

#### Linking Analysis
Both standalone and AU plugin link the same static library:
```ninja
# build.ninja
build element_app_artefacts/Debug/Element.app/.../Element: ...
  LINK_LIBRARIES = ... src/libelement.a ...

build element_instrument_artefacts/Debug/Element_AU.component/.../Element: ...
  LINK_LIBRARIES = ... src/libelement.a ...
```

### Root Cause

**Duplicate JUCE code compiled into multiple libraries**:

The `element` static library incorrectly uses `PUBLIC` linkage to JUCE modules in [src/CMakeLists.txt](../src/CMakeLists.txt):

```cmake
target_link_libraries(element
    PUBLIC
        juce::juce_core
        juce::juce_gui_basics
        juce::juce_audio_basics
        # ... all JUCE modules
    )
```

This causes:
1. **JUCE compiled into libelement.a**: JUCE modules are compiled and archived into `src/libelement.a`
2. **JUCE also compiled into each target**: Standalone app and each plugin format (AU/VST3/LV2) also compile their own JUCE code
3. **AU plugin links BOTH copies**:
   - `libKV-Element_SharedCode.a` (plugin's JUCE code with plugin-specific defines)
   - `src/libelement.a` (contains duplicate JUCE code)

Verification shows both archives contain JUCE:
```bash
$ ar t src/libelement.a | grep juce_audio_basics
juce_audio_basics.mm.o

$ ar t element_instrument_artefacts/Debug/libKV-Element_SharedCode.a | grep juce_audio_basics
juce_audio_basics.mm.o

$ nm -g src/libelement.a | grep "AudioChannelSet.*stereo"
000000000000a138 T __ZN4juce15AudioChannelSet6stereoEv

$ nm -g element_instrument_artefacts/Debug/libKV-Element_SharedCode.a | grep "AudioChannelSet.*stereo"
000000000000a138 T __ZN4juce15AudioChannelSet6stereoEv
```

**The collision**:
1. AU plugin binary contains two copies of `AudioChannelSet::stereo()` and all JUCE statics
2. Standalone app contains JUCE from both its compilation and `libelement.a`
3. Without symbol visibility controls, both AU and standalone export JUCE symbols
4. When AU loads into standalone's address space, dynamic linker resolves to **host's JUCE symbols**
5. AU code executes using host's JUCE static data (different initialization state)
6. Memory corruption results → crash or hang

**Why AU_EXPORT_PREFIX alone is insufficient**:
- `AU_EXPORT_PREFIX=KV_ElementAU` only affects the AudioComponent entry point (`KV_ElementAUFactory`)
- Internal JUCE symbols like `AudioChannelSet::stereo()` remain unprefixed and collide

**Why other formats work**:
- VST3: Uses different isolation mechanism in JUCE's VST3 wrapper
- LV2: Different plugin API, different symbol patterns  
- CLAP: Properly uses `CXX_VISIBILITY_PRESET hidden` (fixed in clap-juce-extensions)
- AudioUnit: Most vulnerable to symbol collision without visibility controls

## Solution

### Required Changes

#### 1. Fix JUCE linkage in static library to prevent duplicate compilation

**File**: `src/CMakeLists.txt`

Change from `PUBLIC` to `INTERFACE` linkage for JUCE modules. This prevents JUCE from being compiled into `libelement.a` while still propagating JUCE modules to targets that link against it:

```cmake
target_link_libraries(element
    PUBLIC
        element-binarydata
        element-luascripts
        element-luamods
        sol2::sol2
    INTERFACE  # Changed from PUBLIC
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
        juce::juce_recommended_config_flags
)
```

**Rationale**:
- `INTERFACE` means "don't compile these into this target, but pass them to targets that link me"
- `libelement.a` will only contain Element-specific code (engine, services, etc.)
- Each plugin format and standalone will compile JUCE once with appropriate defines
- Eliminates duplicate JUCE symbols in AU plugin

#### 2. Add symbol visibility controls (defense in depth)

Even with JUCE compiled only once per target, adding visibility controls ensures complete isolation:

**File**: `src/CMakeLists.txt`

```cmake
# Add after set_target_properties(element PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_options(element PRIVATE -fvisibility=hidden)

set_target_properties(element PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
```

**File**: `CMakeLists.txt` (for each AU plugin target)

```cmake
# After juce_add_plugin and target_link_libraries calls
set_target_properties(element_instrument_AU PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

set_target_properties(element_effect_AU PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

set_target_properties(element_midi_effect_AU PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
```

### Expected Outcome

1. **No duplicate JUCE code**: `libelement.a` contains only Element-specific code, not JUCE
2. **Single JUCE compilation per target**: Each plugin and standalone compiles JUCE once with correct defines
3. **Symbol isolation**: Hidden visibility prevents any remaining symbol leakage
4. **Scanning works**: Out-of-process scanner completes without hanging
5. **Direct loading works**: AU can be loaded without memory corruption
6. **Consistency**: AU behavior matches VST3/LV2/CLAP
7. **Smaller binary sizes**: Eliminates redundant JUCE code in static library

### Verification Steps

1. **Verify libelement.a has no JUCE** (should be empty):
   ```bash
   ar t build/src/libelement.a | grep juce_
   ```

2. **Verify SharedCode.a still has JUCE** (should show juce files):
   ```bash
   ar t build/element_instrument_artefacts/Debug/libKV-Element_SharedCode.a | grep juce_
   ```

3. **Check symbol exports** (should show no or minimal JUCE symbols with visibility):
   ```bash
   nm -g build/element_instrument_artefacts/Debug/libKV-Element_SharedCode.a \
     | grep "AudioChannelSet"
   ```

4. **Test scanning**:
   - Launch Element standalone
   - Scan plugin folders containing Element AU
   - Verify scan completes successfully

5. **Test direct loading**:
   - Load Element AU in DAW or pluginval
   - Verify no crashes or memory errors

6. **Test functionality**:
   - Verify all formats (AU/VST3/LV2/CLAP) continue to work
   - No regressions in existing functionality

## References

### Related Files
- `/Users/mfisher/workspace/kv/element/CMakeLists.txt` - Plugin target definitions
- `/Users/mfisher/workspace/kv/element/src/CMakeLists.txt` - Static library definition
- `/Users/mfisher/workspace/kv/element/src/pluginmanager.cpp` - Plugin scanner implementation
- `/Users/mfisher/workspace/kv/element/src/pluginprocessor.cpp` - Plugin processor

### JUCE Documentation
- AudioUnit format requires proper symbol visibility
- Static data must be isolated between host and plugin
- `AU_EXPORT_PREFIX` only affects entry point, not internal symbols

### Build System
- CMake properties: `CXX_VISIBILITY_PRESET`, `C_VISIBILITY_PRESET`, `VISIBILITY_INLINES_HIDDEN`
- Compiler flags: `-fvisibility=hidden`, `-fvisibility-inlines-hidden`
- Tested with: JUCE 8.0.12, CMake 3.x, macOS

## Conclusion

The hang and crash issues stem from duplicate JUCE code being compiled into both `src/libelement.a` and each plugin's SharedCode library. Using `PUBLIC` linkage for JUCE modules in the static library causes JUCE to be compiled multiple times and linked into the AU plugin twice, creating symbol collisions when the plugin loads into the standalone's address space.

The fix is to change `target_link_libraries(element PUBLIC juce::...)` to `INTERFACE` linkage, ensuring:
1. JUCE is compiled only once per target (standalone, AU, VST3, etc.)
2. `libelement.a` contains only Element-specific code
3. Each target gets JUCE with appropriate compile defines
4. No duplicate symbols exist

Adding symbol visibility controls (`-fvisibility=hidden`) provides additional protection against any remaining symbol leakage.

This follows CMake best practices for static libraries that should propagate dependencies without embedding them.
