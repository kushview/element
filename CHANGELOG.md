# Change Log

## [1.0.0-rc1] - 2024-01-14

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
- **Breaking** Final Script node Lua API has changed. v0.46.x scripts need updated.
- **Breaking** Old 'Lua Node' removed & replaced with Script node instead.

### Added
- Meter bridge view that displays audio interface signal present levels.
- Ability to set a node's color.
- Plugin UI embedding directly in the graph editor.
- Block display modes added: compact and small.
- JACK audio support for Linux, macOS & Windows.
- LV2 plugin support for Linux, macOS & Windows.
- jBridge 32bit bridge direct integration (Windows)
- Drag and drop a plugin from the plugin list on placeholder loads it.
- Bulk routing feature in the patch bay.
- Online update features.
- Better error handling and logging in Lua scripts.

### Removed
- Stop using juce BinaryData from old Projucer project. Resources are now generated with Meson.
- **Breaking**: Controller view to be removed.

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
