# `.element` Extension Format + App-Side Lua Runtime

> **Phase 0 prerequisite:** [session-scripts.md](session-scripts.md) — session-embedded
> hook scripts. `HookBus`, `el.hooks`, the restricted-environment/capability model, and
> `ScriptingService` are built there first; Phase 3 below then reduces to wiring
> `app.*` events and extension-owned registration. See also [luajit.md](luajit.md) for
> the LuaJIT assessment.

## Context

Element has a mature Lua substrate (embedded Lua 5.4, sol2 bindings under `src/el/`, a shared `ScriptingEngine` on `Context`) but the app-side surface is thin: no startup script execution (`ScriptingEngine::execute` is declared but never defined), no Lua graph-mutation API, no lifecycle hooks, and GUI scripting is limited to embedded View scripts. The goal is a new **`*.element` extension format** — a directory (like `.lv2`/`.vst3`) acting as a package/extension that can carry Lua modules, hook scripts, views/panels, DSP scripts, graphs (serialized data **or** Lua builder scripts — manifest decides), presets, resources, and bundled plugins (CLAP first) — plus the app-side Lua runtime it requires: graph building, lifecycle hooks, and GUI extensibility.

**Decisions made with user:**
- Terminology: **Extension** everywhere (classes, service, Lua modules). Directory suffix stays `.element`.
- Install dir: `~/Music/Element/Extensions` (+ app-support equivalent). **Install-only** model — no open-in-place; app scans at startup + rescan command.
- Manifest: **Lua** (`manifest.lua` returning a table), parsed in a restricted environment.
- Builder-script graphs: **single undo transaction**.
- Headers **internal first** (`src/scripting/`), promote to `include/element/` later.
- Full scope planned: format+loader, graph API, hooks, GUI extensibility (views, panels, panel properties) — phased.

## Format spec

```
MyPack.element/
  manifest.lua          -- REQUIRED, returns a table
  init.lua              -- optional entry script (manifest.entry)
  modules/              -- Lua modules exposed to require()
  scripts/              -- DSP / DSPUI / View scripts
  graphs/               -- *.elg data graphs and/or builder *.lua
  presets/              -- *.eln node presets
  plugins/              -- CLAP (later VST3/LV2) binaries
  resources/            -- assets
```

Manifest schema (executed in restricted sol::environment — no `io`/`os`/`require`; side-effect-free at scan time):

```lua
return {
  manifestVersion = 1,
  id      = "com.example.mypack",     -- required, reverse-DNS, stable key
  name    = "My Pack",                -- required
  version = "1.2.0",                  -- required, semver
  author  = "...", description = "...",
  element = { minVersion = "1.0.0" },
  entry   = "init.lua",
  modules = { ["mypack.util"] = "modules/util.lua" },
  scripts = { "scripts/gain.lua" },
  views   = { { slug = "mypack.mixer", title = "Pack Mixer",
                script = "scripts/mixerview.lua", placement = "main" } }, -- main|panel
  graphs  = { { name = "Synth Rig", type = "data",   path = "graphs/rig.elg" },
              { name = "Auto Rig",  type = "script", path = "graphs/autorig.lua" } },
  presets = "presets",
  plugins = { "plugins" },
  resources = "resources",
}
```

C++ side: `struct ExtensionManifest` — plain struct parsed once from the sol table, table discarded (no retained Lua objects). All schema knowledge isolated in `ExtensionManifest::parse(const File&, sol::state_view, ExtensionManifest&)` so the format stays swappable. `Extension` = manifest + `File dir` + status (`discovered/loaded/disabled/error`) + `sol::environment` for the entry script + hook-handle/registration lists for teardown.

## Architecture (new pieces)

| Piece | Location |
|---|---|
| `ExtensionManifest`, `Extension` | `src/scripting/extension.hpp/.cpp` |
| `ExtensionManager` (scan/load/unload registry) | `src/scripting/extensionmanager.hpp/.cpp` |
| `ScriptingService` (new `Service`) | `src/services/scriptingservice.hpp/.cpp` |
| `HookBus` (C++ event dispatcher) | `src/scripting/hookbus.hpp/.cpp` |
| `el.engine`, `el.hooks`, `el.ui` Lua modules | `src/el/Engine.cpp`, `Hooks.cpp`, `UI.cpp` |
| `ViewFactory` (slug → ContentView registry) | `src/ui/viewfactory.hpp/.cpp`, owned by `GuiService` |
| `ScriptContentView` + `MissingExtensionView` | `src/ui/scriptcontentview.hpp/.cpp` |

Key existing seams to reuse (verified):
- `ScriptingEngine::addPackage(name, loader)` runtime package registry ([scripting.cpp:118](src/scripting.cpp#L118)) — extension Lua modules register here (namespaced; no global `package.path` pollution).
- `EngineService` mutation vocabulary ([engine.hpp](include/element/engine.hpp)): `addGraph/addNode/addPlugin/addConnection/connectChannels/connect(PortType,...)/removeNode/disconnectNode` — Lua rides the existing message/undo/engine-sync path.
- `StandardContent::createContentView(const String&)` virtual, consulted first by `setMainView`/`setSecondaryView` ([standard.hpp:88](include/element/ui/standard.hpp#L88), [standard.cpp:541,633](src/ui/standard.cpp#L541)).
- `NavigationConcertinaPanel::addPanel(desc, factory, header)` public ([navigation.hpp:73](include/element/ui/navigation.hpp#L73)); panel state keys by name — extension slugs must be stable.
- `ScriptView` per-view `sol::environment` + descriptor pattern ([scriptview.cpp](src/ui/scriptview.cpp)) — the sandbox precedent for all extension script execution.
- `SessionService::sigSessionLoaded/sigWillSave` ([sessionservice.hpp:37-38](src/services/sessionservice.hpp#L37-L38)), `EngineService::sigNodeRemoved`.
- `Node::parse` tolerant multi-format graph reader ([node.cpp](src/node.cpp)) for `type="data"` providers.

Ownership/lifetime rule: `ExtensionManager` is owned by `ScriptingEngine::Impl` (inside the Lua state's lifetime — sol::environment destruction order). `ScriptingService::deactivate()` unloads extensions **before** state teardown. `ScriptingService` registers in `Services` ctor ([services.cpp](src/services.cpp)) after `EngineService`, before `SessionService`, so extension views/graphs exist before the startup session restores.

## Phases

### Phase 1 — Extension core: format, discovery, module/script loading
- New: `extension.hpp/.cpp`, `extensionmanager.hpp/.cpp`, `scriptingservice.hpp/.cpp`, `test/scripting/extensiontests.cpp`, fixture `test/scripting/fixtures/TestPack.element/`.
- Modified: `src/scripting.hpp/.cpp` (implement dead `ScriptingEngine::execute` as protected `lua.script()` in fresh env returning `Result`; expose `extensions()`), `src/datapath.cpp` + `include/element/datapath.hpp` (`defaultExtensionsDir()` = `~/Music/Element/Extensions`, create in `initializeUserLibrary`; also scan `applicationDataDir()/Extensions`; dev env var `ELEMENT_EXTENSIONS_PATH` mirroring `ELEMENT_SCRIPTS_PATH` handling in [bindings.cpp](src/scripting/bindings.cpp)), `src/services.cpp`, `src/scripting/scriptmanager.cpp/.hpp` (**additive** scan — current `scanDirectory` replaces the registry), `include/element/tags.hpp` (`EL_TAG(Extension)`, `tags::extensionId`, `tags::extensionVersion`, `tags::requires`), `src/CMakeLists.txt`, `test/CMakeLists.txt` (+ `add_test`).
- Load sequence: scan (parse manifests only, no code) → for enabled extensions: register modules via `addPackage`, register DSP/View scripts with `ScriptManager`, run entry script in `sol::environment(lua, sol::create, lua.globals())`; every failure → `logError`, status `error`, never throws out. Enable/disable persisted in `Settings` (`extensionsDisabled` list). `Commands::reloadExtensions` for dev iteration.
- Unload = disconnect hooks, drop env, clear `package.loaded` + registered packages, remove ScriptManager entries and view/panel registrations. Full hot-unload of usertypes is explicitly out of scope (documented).

### Phase 2 — Graph-building Lua API + graph providers
- New: `src/el/Engine.cpp` (`luaopen_el_Engine`), `test/scripting/enginescripttests.cpp`.
- Modified: `src/scripting/bindings.cpp` (register module), `src/el/CMakeLists.txt`, `src/services/scriptingservice.cpp` (provider instantiation), `src/ui/mainmenu.cpp` + commands (File → New Graph From Extension ▸ submenu).
- Facade binds a thin wrapper resolving `EngineService` per call (never bind the service class raw); assert message thread; Lua two-value `nil, "message"` error convention:
  ```lua
  local engine = require ("el.engine")
  local g = engine.addGraph ("My Rig")
  local n = engine.addNode (g, "element.volume")
  local p = engine.addPlugin (g, { format = "CLAP", id = "org.surge..." })
  engine.connect (g, p, 0, n, 0)          -- optional 5th arg "midi" for PortType
  engine.remove (g, n); engine.saveGraph (g, path)
  ```
  `addPlugin` looks up `context().plugins().getKnownPlugins()` by format + identifier/uid/name.
- Providers: `type="data"` → `Node::parse` → re-UUID (same as `.elg` import) → `EngineService::addGraph(node, true)`. `type="script"` → protected run in fresh env, wrapped in a **single UndoManager transaction** (fallback: per-op undo, documented, if bracketing proves infeasible).
- Tests headless with existing `Context` + engine fixtures: builder script topology assertions, data-provider instantiation, plugin-miss returns nil+msg.

### Phase 3 — Hook system
- New: `src/scripting/hookbus.hpp/.cpp`, `src/el/Hooks.cpp`, `test/scripting/hooktests.cpp`.
- Modified: `include/element/engine.hpp` + `src/services/engineservice.cpp` — add `sigNodeAdded`, `sigGraphAdded`, `sigGraphRemoved` at the same sites that fire `sigNodeRemoved`; `scriptingservice.cpp` wires all signals → `HookBus`.
- Events: `app.started`, `app.shutdown`, `session.loaded`, `session.saving`, `graph.added`, `graph.removed`, `graph.changed`, `node.added`, `node.removed`; extensions can `hooks.emit` custom events.
- `HookBus`: message-thread only; handlers are `sol::protected_function` tagged with owner extension id (ExtensionManager sets a "current extension" scope during entry scripts; console registrations = `"user"`); dispatch iterates a copy; reentrancy guard (`dispatching` flag + pending queue); auto-disable a handler after 3 consecutive errors; `removeOwner(id)` on unload.
- Lua: `hooks.on(event, fn) → handle`, `hooks.off(handle)`, `hooks.emit(event, ...)`.

### Phase 4 — GUI extensibility
- New: `viewfactory.hpp/.cpp`, `scriptcontentview.hpp/.cpp`, `src/el/UI.cpp`.
- Modified: `standard.hpp/.cpp` (implement `createContentView` against `ViewFactory`), `src/el/Content.cpp` (finish the `presentView`/`presentViewObject` stubs — string overload resolves real `Content*` via `GuiService`; object overload wraps Lua widget proxies like `ScriptView` does, through the existing `ViewWrapper` in standard.cpp), `guiservice.*` (own ViewFactory, close extension views on `sigExtensionUnloaded`), `bindings.cpp`, `src/el/CMakeLists.txt`.
- `el.ui.registerView { slug, title, script, placement }` → ViewFactory entry producing a `ScriptContentView` (ScriptView machinery fed from an extension file instead of an embedded Script blob) — then `content:presentView("mypack.mixer")` works through the existing name path. `el.ui.registerPanel` → `NavigationConcertinaPanel::addPanel` with a script-backed factory.
- Panel properties (`el.ui.addProperties(panelId, fn)`): `SectionProvider` registries on `GraphSettingsView`/`NodePropertiesView` keyed `"graph.settings"`/`"node.properties"`; first cut limited to text/slider/toggle `PropertyComponent`s backed by Lua get/set callbacks. This is the most invasive piece — do last in the phase or slip to Phase 6.
- Unresolvable view slug → `MissingExtensionView` placeholder, never a crash.

### Phase 5 — Bundled plugins & presets
- Modified: `include/element/plugins.hpp` + `src/pluginmanager.cpp` — `addExtensionSearchPath(format, dir)` (merged into scan paths, **not persisted** to settings), targeted CLAP scan on extension load reusing the existing verified/out-of-process scan path exactly; never scan at manifest-parse time. `src/datapath.cpp`/PresetService: preset discovery additionally walks loaded extensions' `presets/` dirs.
- Duplicate plugin identifiers across extensions: KnownPluginList dedupes, first wins, log.

### Phase 6 — Persistence interplay, management UI, polish
- Session stamping: `requires` child tree on the Session ValueTree listing `{extensionId, version}` for extension-provided content in use; written C++-side during the `session.saving` window. Additive child — no `EL_SESSION_VERSION` bump expected; add a `Session::migrate` no-op guard.
- Graphs instantiated from an extension get informational `extensionId`/`extensionVersion` props; graphs stay self-contained after instantiation (plugin state inlined), so no live dependency unless the extension supplies the plugin binary. Extension DSP-script nodes keep embedding code in the session (existing gzip design) so playback survives a missing extension.
- On session load: diff `requires` vs loaded extensions → one consolidated missing/mismatch alert (compare major version).
- Management: Reload Extensions menu item; extensions list view or preferences page (Plugin-Manager-style) — optional `ExtensionsContentView`.
- Docs: format spec page + a shippable example extension.

## Verification

- Per-phase Boost.Test suites in `test/scripting/` (each registered in `test/CMakeLists.txt` with `add_test`): manifest parse/reject, discovery, `require` resolution, entry-script error isolation (Phase 1); builder-script graph topology + undo-transaction rollback (Phase 2); hook dispatch/auto-disable/removeOwner/reentrancy (Phase 3); ViewFactory register/lookup/missing-placeholder + headless descriptor instantiation (Phase 4); search-path merge + preset discovery from fixture (Phase 5); requires-tree round-trip + missing-extension session load degradation + load→unload→load cycle without duplicate handlers (Phase 6).
- `cmake --build build && ctest --test-dir build --output-on-failure -R Extension...` per suite.
- Manual end-to-end after Phase 4: drop `TestPack.element` into `~/Music/Element/Extensions`, launch app, confirm entry script runs, `New Graph From Extension` builds a graph, hooks fire in the Lua console, and the registered view opens via `presentView`.

## Cross-cutting rules

- All Lua on the message thread only (shared state); engine-side signals already arrive marshalled — assert in HookBus anyway.
- Every extension-code call is protected (`sol::protected_function` / protected script), errors to `ScriptingEngine::logError`, never propagate.
- Extensions are trusted-but-isolated (env per extension inheriting globals, like ScriptView); capability sandboxing out of scope, documented.
- CLAUDE.md compliance: tags via `EL_TAG`, app logic in `ScriptingService`, no `using namespace` in headers, no `ElementApp.h`, Doxygen comments, format with `util/format.py`.
