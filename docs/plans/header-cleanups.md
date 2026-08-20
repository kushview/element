# Public Header Cleanups (`include/element`)

Remaining work from the July 2026 audit of the public header surface. The
interface is considered experimental, so there is no ABI to preserve — this is
the cheapest time to reshape it.

Already done (July 2026): fixed the `ChannelConfig` typed-port accessors in
`porttype.hpp`, the `EL_DISABLE_MOVE` const-rvalue bug in `element.hpp`, the
`AtomicLock` dropped-count issue in `atomic.hpp`, missing `#pragma once` in
`ui/grapheditor.hpp`, misplaced `#pragma once` in `parameter.hpp`, missing
`override` in `ui/decibelscale.hpp` / `ui/simplemeter.hpp`; removed the dead
headers `nodeproxy.hpp`, `datapipe.hpp`, `filesystem.hpp`, `linkedlist.hpp`
and the unused `elPortType` enum from `element.h`.

## Phase 2 — Decide and enforce the public boundary

The root problem: headers are never installed (no `install()`/export rules
anywhere), and `src/CMakeLists.txt` puts both `include/` **and** `src/` on the
PUBLIC include path, so the public/private split is convention only. This is
why app internals have drifted into `include/` unchecked.

- [ ] Decide what the SDK surface actually is. Plausible cut:
  - Core: `Context`/`Services`, the Model layer (`model`, `node`, `session`,
    `graph`, `tags`), extension points (`NodeProvider`/`NodeFactory`,
    `Processor`, `Parameter`, `PortType`).
  - UI contract: `ui/content.hpp` (`ContentFactory`/`Content`/`ContentView`),
    `ui/nodeeditor.hpp`, `ui/updater.hpp`, `ui/about.hpp`, `ui/menumodels.hpp`,
    `ui/view.hpp`, and the `Colors`/`Style` portion of `ui/style.hpp`.
- [ ] Move leaked app internals from `include/element/` to `src/`:
  `application.hpp`, `ui/standard.hpp`, `ui/simplemeter.hpp`,
  `ui/decibelscale.hpp`, `ui/designer.hpp`, `ui/grapheditor.hpp`,
  `ui/meterbridge.hpp`, `ui/popups.hpp`. All are concrete components used only
  inside `src/` and are not part of any factory contract.
- [ ] Enforce the boundary in CMake: make `src/` a PRIVATE include dir of the
  `element` target, declare public headers with `FILE_SET HEADERS`, and add
  `install()` + export-set/package-config rules so the SDK is actually
  shippable. Enforcement is what stops the drift from recurring.
- [ ] Verify with a clean configure/build that nothing outside the target
  reached headers only via the removed PUBLIC `src/` include dir.

## Phase 3 — Stop third-party types leaking through public signatures

- [ ] Decide deliberately whether Boost.Signals2 is part of the public
  contract. If yes, document it; if no, wrap it — `signals.hpp` is already the
  single choke point (`element::Signal` is used in `processor.hpp`,
  `engine.hpp`, `audioengine.hpp`, `session.hpp`, `midiiomonitor.hpp`).
  `parameter.hpp` uses `boost::signals2::signal` directly and should go
  through the alias either way.
- [ ] `version.hpp` includes `<boost/algorithm/string.hpp>` just for
  split/trim — replace with a small local helper.
- [ ] Keep the Lua C API out of public signatures: pimpl `LuaMidiPipe`
  (`midipipe.hpp` exposes `lua_State*`), and drop `script.hpp`'s include of
  `lua.hpp` (which pulls raw `lua.h`/`lauxlib.h`/`lualib.h`).
- [ ] `processor.hpp` includes the whole `<element/juce.hpp>` umbrella (with a
  self-flagged FIXME) — replace with the specific module headers it needs.
- [ ] `juce/core.hpp` injects `element::` aliases for juce types
  (`// FIXME: juce aliases`) — remove or move them out of the umbrella.
- [ ] `shuttle.hpp` uses the deprecated
  `juce::AudioPlayHead::CurrentPositionInfo` — migrate to `PositionInfo`.

## Phase 4 — Longer-term header quality

- [ ] Pimpl the god headers. `processor.hpp` (~22 KB: full private state,
  nested `RMSMeter`/`MidiProgramLoader`/`PortResetter` structs, 6 friends) is
  the worst offender; also `node.hpp` (`NodeObjectSync`, fully-inline
  `ConnectionBuilder`), `ui.hpp` (`GuiService` has an `Impl` yet still exposes
  window/content members and nested structs), `session.hpp` (friends + leaked
  ValueTree helpers), `plugins.hpp` (`PluginScanner` private members). The
  pattern already exists in-tree (`Context`, `AudioEngine`, `NodeFactory`,
  `Services`).
- [ ] Move large inline bodies to `.cpp`: the `Commands`
  `toString`/`fromString`/`getAllCommands` tables in `ui/commands.hpp`
  (~250 lines), and the `LookAndFeel_E1` override wall in `ui/style.hpp`.
  While moving the command tables, fix the `toString`/`fromString` asymmetry —
  several command IDs (e.g. session*/transport*) don't round-trip.
- [ ] Pick one export-macro policy and apply it uniformly. `EL_API` currently
  decorates about half the public types; `arc.hpp` even mixes `JUCE_API`
  (`Arc`) with `EL_API` (`ArcSorter`/`ArcTable`). Consider a generated export
  header (CMake `GenerateExportHeader`).
- [ ] `tags.hpp`: `EL_TAG` expands to `static const juce::Identifier` at
  namespace scope, so every translation unit gets a private copy of every
  Identifier. Switch to `extern` declarations with a single definition TU
  (or `inline` variables).
- [ ] `model.hpp`: `EL_MODEL_GETTER`/`EL_MODEL_GETTER_WITH_TYPE`/
  `EL_MODEL_SETTER` are never `#undef`'d and leak into every includer
  (contrast `tags.hpp`, which `#undef`s `EL_TAG`).
- [ ] Reframe the C API honestly. `element.h`'s live role is the export-macro
  layer (`EL_PLUGIN_EXPORT` on the `luaopen_el_*` entry points) plus the
  `EL_MT_*` Lua metatable names — there are no handles, descriptor structs, or
  versioning, so it is not a C ABI. Either commit to a real C ABI or document
  `element.h` as "export + Lua-module macros".
- [ ] Doc pass over the undocumented keep-public headers: `context.hpp`,
  `devices.hpp`, `settings.hpp`, `oversampler.hpp`, `audioengine.hpp`,
  `ui/nodeeditor.hpp`, `ui/style.hpp`. (`transport.hpp`, `taptempo.hpp`,
  `parameter.hpp`, `plugins.hpp`, `ui/updater.hpp` are the in-tree examples of
  the standard to match.)
- [ ] Settle the getter naming convention for new code (`getName()` in
  `Node`/`Session` vs `name()` in `Model`/`Script`/`PortType`) and document it
  in `docs/cppstyle.md`.
- [ ] Unify the remaining copyright-header variants when files are otherwise
  touched (don't rewrite years wholesale).
