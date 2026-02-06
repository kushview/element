# Change Log

## [1.0.0] - 2026-02-06

### Changed
- Update versioning to include project version in build number.
- Refactor updater interface to use feed URL instead of repository.
- Remove build version suffix for consistency.
- Use JUCE and Sol2 system packages when available.
- Extract sol2 repository based on CMake version.
- Stop using ElementApp.h.
- Reduce binary data size by excluding unused resources.
- Remove alias headers and update client code.
- Improve symbol visibility and export properties for plugin loading.
- Use standard plugin locations on Linux for installation.
- Refactor appcast URL definition for consistency and clarity.

### Added
- Sparkle updater integration for macOS.
- WinSparkle updater support for Windows.
- Background update checks with improved documentation.
- Shutdown handling for updates.
- Update checks prevented on first run.
- Feed URL for updater with availability logging.
- macOS uninstaller target to build Uninstall.app from AppleScript.
- Spoton Scale Chooser program.
- Build and REUSE status badges to README.
- REUSE compliance tagging.

### Removed
- JUCE submodule (now uses system package or FetchContent).
- LVTK submodule and all references to internal LV2/LVTK implementation.
- Unused Element modules and legacy code.
- Obsolete updater documentation.
- Unused strings.cpp file.
- Duplicate includes and unused headers.

### Fixed
- MSVC 14.5+ compilation error in Point.cpp by isolating C linkage from C++ lambda code.
- Ensure IONode's parent graph has minimum port count on setParentGraph.
- Correctly handle parameter sync from plugin to host in CLAP.
- Don't create audio delay ops for control ports on single connection.
- Warnings from JUCE about parameter IDs not being set.
- Hide check updates preference when updater disabled.
- JACK on Windows to use system library loader and symbol lookup functions.
- Linux to correctly acquire exe full path when launched by PATH.
- Remove duplicate include of datapath.hpp in datapathbrowser.hpp.
- Code signing with JUCE CMake helpers.
- Replace hardcoded ASIO SDK path with FetchContent for reproducible builds.

## [1.0.0b2] - 2026-01-06

### Changed
- Update to JUCE 8.0.12.
- Migrate to CMake build system. Meson is no longer used.
- Improve MIDI clock sync algorithm for faster BPM updates.
- Refactor Application class to its own file and allow subclasses to create ContentFactory.
- Update URL macros to use ELEMENT_ prefix for consistency.
- Add option to enable updater support in CMake configuration.

### Added
- CMake build system support with comprehensive configuration options.
- GitHub Actions CI workflows for build, test, and release on Mac/PC/Linux.
- Arch Linux Docker build support and workflow.
- Linux desktop file installation support.
- Unit tests for DelayLockedLoop, MidiClock, AlignedData, AtomicValue, and AtomicLock.
- Comprehensive audio routing unit tests.
- Apply button to IOConfigurationWindow.
- Lua documentation generation with ldoc.
- OSC send utility for testing audio engine.
- Support for ccache to speed up builds.

### Removed
- Qt Installer framework references (deprecated).
- Meson build system files and configurations.
- Unused nodeioconfiguration files.
- Old deploy directory.

### Fixed
- Fix race condition in script reload causing null pointer crash.
- Fix MIDI clock and message collector assertions when audio device not running.
- Fix save prompt on clean project load.
- Convert file choosers to async to prevent crash.
- Fix GCC 15 build error with incomplete type in std::unique_ptr.
- Fix device availability checks in MidiDeviceProcessor.
- Fix AU parameter ID problems.
- Fix compile error when binding lua Point:rotated.
- Fix macOS file system watcher implementation.
- Fix paths on Windows for LV2 plugins.
- Fix problems scanning when crashed plugins file doesn't exist.
- Fix restoration of desktop scale.
- Fix IO node port labels to match containing graph IO.
- CLAP: Fix note off not being handled correctly in synths.
- CLAP: Send time information to plugins.
- LV2: Add port-resize support for Atom output ports.
- eVerb: Use buses layout support instead of old play config method.
- Audio Router Node: Actually size to 12 channels when selected.
- Audio Mixer: Fix crashing when trying to change bus size.
- Audio Mixer: Ensure temp buffer gets sized appropriately before clearing.
- Scanner: Apply default plugin paths on first run.
- Scanner: Use 24 second timeout.
- Generic UI: Show output parameters.

## [1.0.0b1] - 2025-02-04

### Changed
- Migrate to Meson build system.  Projucer no longer needed.
- Migrate to BOOST unit testing.
- Major code refactoring in preparation for future public C++ api and loadable modules.
- Graph Editor - Allow editing graphs that aren't active.
- UI General look and feel improvements.
- Plugin scanner XML now stored in its own file.
- Root graph channel counts are now independent fromt he audio interface.
- Improve multiple selection in graph editor.
- Updated app icon.
- Session, Graph and Node file formats.  Old files can be loaded in 1.0, but 1.0 can't be backported. Session backup is strongly encouraged.
- Internal 'presets' are now called 'nodes.'
- **Breaking** The Script node Lua API has changed. v0.46.x scripts need updated and may not load.

### Added
- Meter bridge view that displays audio interface signal present levels.
- Ability to set a node's color.
- Plugin UI embedding directly in the graph editor.
- Block display modes added: compact and small.
- JACK audio support for Linux, macOS & Windows.
- LV2 plugin support for Linux, macOS & Windows.
- CLAP plugin support for Linux, macOS & Windows.
- jBridge 32bit bridge direct integration (Windows)
- Drag and drop a plugin from the plugin list on placeholder loads it.
- Bulk routing feature in the patch bay.
- Online update features.
- MIDI Set List node with Tempo change.
- Better out-of-process plugin scanning.
- Repo selection built-in with installer.
- Audio File Player - better transport controls.
- Embedded plugin UI display inside graph editor.
- Ability to scan plugins from Element plugins.

### Removed
- Stop using juce BinaryData from old Projucer project. Resources are now generated with Meson.
- **Breaking**: Controller view are hidden by default. MIDI Mapping is to be re-written.
- **Breaking**: Old 'Lua' and 'Script' nodes have been deprecated. Rewrite planned.

### Fixed
- Plugin UIs not filling the plugin window (WAVES and others)
- Plugin UI general display problems.
- Disruptions in audio when enabling Oversampling.
- Zombie process created by the Linux plugin scanner.
- Graph mixer reseting faders to unity gain when selecting.
- Generic parameters not syncing with performance parameters in the plugin version
- Several LV2 problems. Namely CV, patch, and atom support.
- Custom keymappings not retaining state.
- Inverse sustain and Hold buttons in Virtual keyboard.
- Element plugins rendering silence in some hosts.
- Audio File Player: crashing when loading files.
- Broken caps-lock virtual keyboard function.
- Audio Router Node: Mute wasn't working.
- Audio Router Node: disconnecting when reopening saved sessions.
- Broken channel deselection in audio preferences.
- Drag and drop sometimes failing to load plugins.
- MIDI monitor not logging messages after extended use.
- Plugin not reporting latency to the host correctly. Especially during graph editing.
- 7.1 and other bus configurations on third party plugins not saving/restoring with session.
- Broken cubase render in place.
- Problems loading copy protected plugins.
- eVerb state not being applied on state restore.
