# Element manual: code issues found while documenting

UI that exists but does not work, or that contradicts what the manual says, is
left out of the manual and listed here so the code can be fixed and the doc
updated afterwards. Each entry names the source location as of the `manual`
branch (September 2026). Remove an entry when the fix lands and the manual is
updated.

## Visible but non-functional

- **Edit > Cut / Copy / Paste** have no command handler and are always
  disabled. `src/ui/mainmenu.cpp`, `include/element/ui/commands.hpp`.
- **Edit > Insert plugin...** (Cmd+P) is hard-coded disabled.
- **Preferences "Plugins" page** ("Enabled Plugin Formats") is never added to
  the window: it is missing from `addDefaultPages()` and the copy inside the
  General page is never added as a child. `src/ui/preferences.cpp:575`.
- **Plugin window "M" (mute) button** is created but never positioned, so it
  never shows. `src/ui/pluginwindow.cpp`.
- **Graph editor zoom / scrolling**: the zoom code exists but nothing calls it;
  "Gather nodes..." is the only way to recover off-screen blocks.
  `src/ui/grapheditorcomponent.cpp`.
- **Import Session wizard** (`src/ui/sessionimportwizard.cpp`) is never opened.
- **OSC `/element/command`** is parsed but does nothing.
  `src/services/oscservice.cpp:34`. The old manual listed ~25 command
  addresses under it; they are removed from the OSC appendix.
- **Debug menu "Show/Save/Load Workspace"** items do nothing (debug builds
  only).

## Wrong behaviour

- **Virtual keyboard Sustain/Hold pedals are swapped**: "Sustain" sends CC 66
  (sostenuto) and "Hold" sends CC 64 (sustain).
  `src/ui/virtualkeyboardview.cpp:136,151`.
- **OSC Receiver converts `channelPressure` to a note-off** instead of a
  channel-pressure message. `src/utils.hpp` (around line 357).
- **View > Session Properties** works, but the `showSessionConfig` case in
  `src/ui/standard.cpp:1025` has no `break`, and the Key Mappings editor shows
  the command as "Graph Settings".
- **`Element Debug` / dev**: "Quick Map" and "Refresh Mapping Engine" are
  debug-only; not documented.

## Stale text and labels

- **Graph Settings key-map button** still reads "Map"; the toolbar and command
  are "MIDI Learn" / "learn". `src/ui/graphsettingsview.cpp:292`.
- **"UI Type"** preference has exactly one value ("Standard").
  `src/ui/preferences.cpp:479`, `src/settings.cpp` (`mainContentType`).
- **LADSPA** is listed in `data/intro.txt` and in the old manual, but CMake
  never enables it.
- **CLAP** is missing from the format lists in `data/intro.txt` and the old
  introduction.
- **ldoc link for `el.AudioBuffer`** points at the old `kv.AudioBuffer` page
  (`docs/manual/scripting/script-types.rst`, top of file).
- **`persistent` graph flag** is set on every node but no longer used.

## Missing default shortcuts

No default key binding for: Plugin Manager, Key Mappings (its binding is
commented out), Channel Strip, Graph Mixer. `src/services/guiservice.cpp:779`,
`src/ui/standard.cpp:996`. The "Rewind" (J) and "Forward" (L) transport
commands have keys but no menu item and no toolbar button.

## Plugin builds

- Only the *default* key bindings work inside the plugin editor; custom
  bindings from Key Mappings are ignored. `src/plugineditor.cpp:419`.
