# Element manual: screenshots needed

Every figure the manual needs but does not have yet is marked in the RST source
with a comment of the form `.. SCREENSHOT <id>: <what to capture>` directly
above a commented-out `figure` directive. This file is the working list for
taking them. Keep the two in sync:

```
grep -rn "^.. SCREENSHOT" docs/manual --include=*.rst
```

Images live under `docs/manual/images/<part>/<page>-<nn>.png`. Capture at 100%
desktop scale on a light theme, crop to the element described, and keep the
main-window shots at 1230x730 (the default window size) unless the row says
otherwise. Once an image is in place, uncomment the `figure` block, delete the
marker, and change the row's status to `done`.

| ID | Page | What to capture | Setup / state | Size | Status |
|----|------|-----------------|---------------|------|--------|
| using/preferences-00 | using/preferences | Preferences, General page | Default settings | dialog | retake (labels changed since capture) |
| using/preferences-01 | using/preferences | Preferences, Audio page | A real device selected, Experimental group visible | dialog | retake (Experimental group is new) |
| using/preferences-02 | using/preferences | Preferences, MIDI page | At least one MIDI input enabled | dialog | retake (MIDI Panic CC row is new) |
| using/preferences-03 | using/preferences | Preferences, OSC page | OSC host enabled | dialog | exists |
| using/preferences-04 | using/preferences | Preferences, Updates page | Stable channel, not signed in | dialog | retake (page redesigned: Release Channel + Sign in) |
| using/plugin-manager-00 | using/plugin-manager | Plugin Manager view | Several plugins listed across formats, one favourite | main view | exists |
| getting-started/quick-start-01 | getting-started/quick-start | Main window right after first launch | Fresh settings, empty graph, sidebar visible | 1230x730 | needed |
| getting-started/quick-start-02 | getting-started/quick-start | Instrument block wired from MIDI In and to Audio Out | One instrument plugin, vertical orientation, Normal display | graph editor crop | needed |
| getting-started/concepts-01 | getting-started/concepts | Session panel showing two root graphs, one expanded with nodes and a subgraph | Session with two graphs, a subgraph in the first | sidebar crop | needed |
| interface/main-window-01 | interface/main-window | Whole main window, all areas visible | Small session; Session + Node panels open; graph editor; Graph Mixer accessory; virtual keyboard shown | 1230x730 | needed |
| interface/menus-and-commands-01 | interface/menus-and-commands | Key Mappings view | One category expanded | main view crop | needed |
| interface/graph-editor-01 | interface/graph-editor | Graph editor with 5-6 blocks, one in Embed mode, cables, breadcrumb | Vertical orientation | main view | needed |
| interface/graph-editor-02 | interface/graph-editor | Background context menu open | Shows Graph I/O, device submenus, Plugins section | menu crop | needed |
| interface/graph-editor-03 | interface/graph-editor | Block context menu open with Presets submenu expanded | Plugin node with at least one user preset | menu crop | needed |
| interface/patch-bay-01 | interface/patch-bay | Patch bay with several nodes and a few connections | Route and Clear visible | main view | needed |
| interface/mixer-strip-meters-01 | interface/mixer-strip-meters | Graph Mixer with 4-5 strips | One selected, one coloured block | accessory view crop | needed |
| interface/session-tree-01 | interface/session-tree | Session panel with two graphs | Active graph highlighted; one expanded with a subgraph and a script node showing DSP/UI | sidebar crop | needed |
| interface/node-panel-01 | interface/node-panel | Node panel + Editor panel | Instrument plugin, MIDI section, two saved programs | sidebar crop | needed |
| interface/plugin-windows-01 | interface/plugin-windows | A plugin window over the main window | Toolbar buttons visible | window | needed |
| using/graphs-01 | using/graphs | Graph panel with every property | MIDI Program set, hotkey mapped | sidebar crop | needed |
| using/midi-mapping-01 | using/midi-mapping | MIDI Mappings view | Several mappings, one selected with its settings | main view | needed |
| using/virtual-keyboard-01 | using/virtual-keyboard | Virtual keyboard strip | A few keys held | bottom strip crop | needed |
| nodes/audio-01 | nodes/audio | EQ Filter editor | Plugin window, Bell shape | window | needed |
| nodes/audio-02 | nodes/audio | Audio Router editor at 4x4 | Two routes set | window | needed |
| nodes/midi-01 | nodes/midi | MIDI Set List editor | Three songs with tempo and signature | window | needed |
| scripting/script-node-01 | scripting/script-node | Script node window running Amp | DSPUI editor and Params table shown | window | needed |
| scripting/console-01 | scripting/console | Console in the accessory view | A few commands with results | accessory view crop | needed |
| plugin/editions-01 | plugin/editions | Element FX editor inside a host | Eight performance sliders, one right-click menu open | window | needed |
