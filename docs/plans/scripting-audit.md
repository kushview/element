# App-Side Scripting — Audit (September 2026)

State of Element's Lua scripting *outside* the ScriptNode, taken before building the
console REPL and the hook system. Companion docs: [session-proxy.md](session-proxy.md)
(the mutation route), [session-scripts.md](session-scripts.md) (Phase 0: hooks, console,
session scripts), [extensions.md](extensions.md), [luajit.md](luajit.md).

ScriptNode (`src/nodes/scriptnode*`) has its own `sol::state` and its own lifecycle and is
out of scope here.

## Verdict

| Layer | State | Evidence |
|---|---|---|
| Lua core: vendored 5.4 + sol2, `el.*` module searcher, `ScriptLoader`, `DSPScript` | **Solid** | Tested in `test/scripting/*`; `ScriptLoader` and `DSPScript` are exercised by ScriptNode in production. |
| Model bindings `el.Session` / `el.Node` / `el.Graph` | **Solid, read-mostly** | Userdata wraps the *live* tree (`std::make_shared<Node> (tree, false)`, `SessionPtr`), not copies. Can set name/tempo and save/restore state, nothing else. |
| App-side scripting: `ScriptingEngine`, `ScriptManager`, console, `el.command`, `el.Content` | **Flaky — abandoned mid-build** | Console bootstrap fails silently in dev builds, `el.command` calls a method that does not exist, several declared-but-undefined or permanently dead members (below). |
| Sandboxing | **None** | `Lua::initializeState` opens every stdlib. `ScriptView` creates a `sol::environment` and never applies it, so View scripts embedded in a session run in raw globals with `io`/`os`/`debug`. |
| Event / hook layer | **Absent** | No registry, no emit, no priorities. The signals that exist are asymmetric and incomplete. |

## 1. The Lua state

- One root `sol::state` per `Context`, owned by the pimpl `ScriptingEngine::State`
  ([scripting.cpp:17-49](../../src/scripting.cpp#L17)). `Context::Impl::init()` creates
  the engine and calls `initialize (owner)` ([context.cpp:80-81](../../src/context.cpp#L80)).
  Reached via `context().scripting().getLuaState()`.
- Init is two-phase and partly duplicated: the `State` ctor opens `base`+`string` and
  installs a searcher `resolve_internal_package` whose `builtins`/`packages` maps are
  **never filled** (`fill_builtins` is commented out, `addPackage` has no callers). Then
  `Lua::initializeState` ([bindings.cpp:413-435](../../src/scripting/bindings.cpp#L413))
  opens *all* libraries, inserts the real searcher `searchInternalModules` (a 160-line
  if/else chain kept in sync by hand with ~30 `extern "C"` declarations) at position 2
  of `package.searchers`, and sets `package.path`, `package.cpath` (always empty) and the
  non-standard `package.spath`. Both searchers coexist: `resolve_internal_package` is
  pushed to position 3, so anything registered through `addPackage` *does* resolve — the
  map is simply empty because nothing calls it.
- `_G["el.context"]` is a raw `std::ref<Context>` ([bindings.cpp:402-405](../../src/scripting/bindings.cpp#L402)),
  nilled in `~ScriptingEngine`. Nothing guards Lua values that captured it earlier.
- No thread assertions anywhere in `src/scripting/`, `src/el/`, `src/scripting.cpp`.
  Everything app-side runs on the message thread by convention only.

## 2. The console today

Classes: `Console` ([console.hpp](../../src/ui/console.hpp)) → `LuaConsole`
([luaconsole.hpp](../../src/ui/luaconsole.hpp)); `LuaConsoleView`
([luaconsoleview.hpp](../../src/ui/luaconsoleview.hpp)) is the `ContentView` that owns a
`LuaConsole` and listens to `Log`. Shown only as the secondary (bottom) pane via
`Commands::showConsole` ([standard.cpp:1154-1163](../../src/ui/standard.cpp#L1154)),
instantiated at [standard.cpp:644-647](../../src/ui/standard.cpp#L644). No feature flag.

Eval chain: `ConsolePrompt::onReturnKey` → `Console::handleTextEntry` →
`LuaConsole::textEntered` ([luaconsole.cpp:25-73](../../src/ui/luaconsole.cpp#L25)):
compile-probe `"return <text>;"`, else run the raw text, through
`sol::state_view::script (code, env, "console=")`. The env is created once per view:
`sol::environment (view, sol::create, view.globals())`
([luaconsoleview.cpp:21-26](../../src/ui/luaconsoleview.cpp#L21)) — reads fall through
to `_G`, writes stay local. `print` is an env-local lambda pushing to a `StringArray`
drained by a `Timer`. Up/Down history (100 entries, not persisted).

Reaching the session works right now:

```lua
local ctx = require ('el.Context').instance()   -- reads _G["el.context"]
local s = ctx:session(); print (#s, s[1].name)
```

Defects, in priority order:

1. **Prelude fails silently.** `setEnvironment` runs
   `require('el.script').exec('console', _ENV)`. `el.script.exec` *returns* the error
   string instead of raising ([script.lua:30-34](../../src/el/script.lua#L30)), so the
   `valid()` check never fires. `scripts/console.lua` is looked up on `package.spath`
   (`~/Music/Element/Scripts`, app-data `Scripts`,
   `Element.app/Contents/Resources/Scripts`); none exist in a dev build and
   `scripts/CMakeLists.txt` only installs to `share/element/scripts`. The scripts *are*
   already compiled into binary data (`scripts::console_lua` in `luascripts.hpp`) but
   only ScriptNode uses that.
2. **`el.command` is dead.** [command.lua:10-13](../../src/el/command.lua#L10) calls
   `Context.instance():commands()`; `el.Context` binds no such method
   ([Context.cpp:39-58](../../src/el/Context.cpp#L39)) and `Context` has no accessor —
   `Commands` lives on `GuiService::Impl`. Every `command.invoke` asserts.
   `scripts/commands.lua` requires a non-existent `el.CommandManager`.
3. **No topology mutation API.** Nothing in `src/el/` calls
   `addNode/addPlugin/addConnection/removeNode/addGraph`.
4. **State dies on toggle.** `showConsole` deletes the view; env, history and buffer are
   rebuilt each time.
5. **Threading.** `LuaConsoleView::messageLogged` is called from `Log::logMessage` on the
   *caller's* thread inside `Log`'s lock and writes a `TextEditor`. `printMessages` is a
   plain `StringArray` shared between the Lua `print` lambda and the timer.
6. `_G.print` is swapped for the duration of every eval
   ([luaconsole.cpp:33,71](../../src/ui/luaconsole.cpp#L33)); re-entrancy (modal,
   `os.exit`) leaves it swapped. `lastError` is never assigned and `errorHandler` is
   declared but never defined. The `else` error branch is unreachable (sol throws by
   default). `buffer.length()` (characters) is passed as the byte length to
   `load_buffer`. No `debug.traceback`. `EL_VIEW_CONSOLE` is `"LuaConsoleViw"` and is
   persisted to settings. No default keypress. `console.log` in the prelude writes to
   stdout, not the console.

## 3. `ScriptingEngine` / `ScriptManager` dead weight

| Item | Where | Fact |
|---|---|---|
| `ScriptingEngine::execute (const String&)` | [scripting.hpp:28](../../src/scripting.hpp#L28) | Declared, never defined. |
| `lua_State* L` | [scripting.hpp:40](../../src/scripting.hpp#L40) | Never assigned. |
| `State::builtins`, `fill_builtins` | [scripting.cpp:84-126](../../src/scripting.cpp#L84) | Map never filled; the fill call is commented out. Dead. |
| `State::resolve_internal_package`, `packages`, `addPackage` | [scripting.cpp:64-126](../../src/scripting.cpp#L64) | **Live**, not dead: the searcher stays at position 3 of `package.searchers`. No callers yet; [extensions.md](extensions.md) Phase 1 registers extension modules through it. |
| `EL_LUA_SPATH` | [scripting.cpp:10](../../src/scripting.cpp#L10) | Defined, never referenced. |
| `ScriptManager` | [scriptmanager.cpp](../../src/scripting/scriptmanager.cpp) | Never scans in the app: `Application::setupScripting` is `ignoreUnused (scripts)`; `Impl::scanDefaultLoctaion` (sic) has no callers. Test-only. |
| `ScriptInstance::object` (keep the class) | [scriptinstance.hpp](../../src/scripting/scriptinstance.hpp) | Private, no setter, so `cleanup()` is unreachable. |
| `DSPUIScript`, `ScriptSource` | `src/scripting/` | No users *yet*: intentional scaffolding for the script-type hierarchy and for where script code is sourced from (`ValueTreeScriptSource` → session scripts). Keep. |
| `DSPScript::validate` | [dspscript.cpp:434-438](../../src/scripting/dspscript.cpp#L434) | Returns `ok()` for any non-empty string; real body is `#if 0`. |
| `el.vector` | `src/el/vector.c` | Compiled, not registered in `searchInternalModules`. |
| `widget.hpp` `__props` | `src/el/widget.hpp` | Missing comma fuses `"visible" "opaque"` into `"visibleopaque"`. |
| `el/session.lua` | [session.lua:12](../../src/el/session.lua#L12) | Caches the `Session` userdata at require time; stale after reload. |
| `ScriptView::Impl::env` | [scriptview.cpp:24,71-78](../../src/ui/scriptview.cpp#L24) | Created, never passed to `loader.call()`; View scripts run in raw globals. |
| `getLuaCPath`, `getLocalScriptsDir`, `getLocalLuaDirs` | [bindings.cpp](../../src/scripting/bindings.cpp#L79) | Stubs returning empty. `EL_LUADIR`/`EL_SCRIPTSDIR`/`LUA_PATH_DEFAULT` are branched on but defined by no CMake file. |

## 4. `el.*` bindings inventory

Lua-source modules (compiled via `src/el/CMakeLists.txt` into `luamods.hpp`):
`el.AudioBuffer`, `el.object` (the proxy/OO system used by widgets), `el.script`
(loader + type constants), `el.session` (two helpers), `el.command` (broken),
`el.strings`, `el.color` (empty).

C/C++ modules: `el.Context`, `el.Session`, `el.Node`, `el.Graph`, `el.Commands`,
`el.Content`, `el.View`, `el.GraphEditor`, `el.MidiPipe` (defined in
`src/engine/midipipe.cpp`), `el.Widget`, `el.TextButton`, `el.Slider`, `el.Desktop`,
`el.Graphics`, `el.MouseEvent`, `el.Bounds`/`Rectangle`/`Point`/`Range`,
`el.AudioBuffer32/64`, `el.MidiBuffer`, `el.MidiMessage`, `el.audio`, `el.midi`,
`el.bytes`, `el.round`; experimental `el.DocumentWindow`, `el.File`.

What the model bindings can do today:

- `el.Session` ([Session.cpp](../../src/el/Session.cpp)): `#s`, `s[i]` (live child
  wrapped as `Node`), `name` and `tempo` get/set, `toXmlString`, `saveState`,
  `restoreState`.
- `el.Node` / `el.Graph` ([nodetype.hpp](../../src/el/nodetype.hpp)): `name` get/set,
  `valid`, `displayName`, `pluginName`, `uuidString`, `nodeId`, `nodeType`, `isGraph`,
  `isRoot`, `toXmlString`, `saveState`/`restoreState`, `missing`, `enabled`,
  `bypassed`, `muted` (read-only), `writeFile`, `resetPorts`, `hasEditor`,
  `hasViewScript`, `viewScript`. The index metamethod returns every child as a plain
  `Node`, even graphs.
- `el.Commands`: `invokeDirectly`, static `standard()`, `toString()`. No way to obtain
  an instance.
- `el.Content`: `showToolbar`, `presentView (name)`; `presentViewObject` is an empty stub.

## 5. Notification plumbing

`include/element/signals.hpp` is three `boost::signals2` aliases. Signals that describe
a model or engine change:

| Signal | Emitted from | Gap |
|---|---|---|
| `SessionService::sigSessionLoaded` | `refreshOtherControllers()` ([sessionservice.cpp:321-328](../../src/services/sessionservice.cpp#L321)) **and** directly from [pluginprocessor.cpp:800](../../src/pluginprocessor.cpp#L800) | The plugin path skips `EngineService::sessionReloaded()` and `MappingService::refresh()`. |
| `SessionService::sigWillSave` | `saveSession` | — |
| `EngineService::sigNodeRemoved` | `removeNode (const Node&)`, `removeGraph` | Not fired by `removeNode (uint32)` or by `GraphManager::removeNode`. |
| `GuiService::nodeSelected`, `sigRefreshed` | UI | `sigRefreshed` has no subscribers. |
| `DeviceService::sigMidiDevicesChanged`, `sigAudioDeviceStatus` | devices | — |

Missing entirely: node added, graph added/removed/moved/activated, connection
added/removed, session closed/new.

- `Session` is a `ChangeBroadcaster`; its four `ValueTree::Listener` overrides all
  collapse into one undifferentiated `notifyChanged()`
  ([session.cpp:255-294](../../src/session.cpp#L255)). Its only listener is
  `SessionDocument` (dirty flag).
- `GraphManager::changed()` is a payload-less `sendChangeMessage()`.
- `AppMessage` types ([messages.hpp](../../src/messages.hpp)) are dispatched by a
  `dynamic_cast` chain in `Services::handleMessage`; only `GuiService` overrides
  `handleMessage`.
- Commands are integer ids and a `switch` in `GuiService::perform`; no runtime
  registration. `Commands::toString` is incomplete, which silently truncates the
  constants `command.lua` generates.
- Threading: all `Service` methods and all signals above run on the message thread.
  Engine-side state changes reach the message thread through `AsyncUpdater`
  (`RootGraphRender`, `GraphNode`, `Processor::EnablementUpdater`,
  `AudioProcessorParameterCapture`, `MappingEngine`).

## 6. Tests

`test/scripting/`: `ScriptLoaderTest`, `DSPScriptTest`, `PresetScriptsTest`,
`MidiScriptTests`, `BytesTest`, `ScriptInfoTest`, `ScriptManagerTest`, `ScriptPlayground`.

- `LuaFixture` ([luatest.hpp](../../test/scripting/luatest.hpp)) stands up a bare
  `sol::state` with the *no-Context* `initializeState`, so `el.Context`, `el.Session`,
  `el.command` and anything needing the app are untestable through it. `resetPaths()`
  is CWD-relative while `getSnippetFile()` uses `EL_TEST_SOURCE_ROOT`.
- `test/TestMain.cpp` provides `element::test::context()`, which constructs a `Context`
  and activates services — the right fixture for proxy/hook tests.
- `ScriptManagerTest` asserts a hard-coded `getNumScripts() == 13`.
- `test/snippets/sol3_parent.lua` and `stream_from_c.lua` are orphaned.

## Not verified in this audit

- Whether any shipped installer places `scripts/*.lua` in the macOS bundle
  (`Contents/Resources/Scripts`); only the CMake install rule and the dev bundle were
  checked.
- Behaviour of `DSPScript::process`'s unprotected `lua_call` on error with Lua built as
  C (`longjmp` vs C++ exceptions) — noted, not exercised.
