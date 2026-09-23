# Phase 0 — Console, Hooks & Session Scripts

Predecessor to [extensions.md](extensions.md). Front-loads what the extension format
needs anyway — the hook bus, the script descriptor conventions, the restricted
environment / capability model — and delivers them first for the Lua **console** and for
scripts **embedded in the session file**, where DSP/DSPUI scripts already live today.

Prerequisites and companions:

- [scripting-audit.md](scripting-audit.md) — what exists today and what is broken.
- [session-proxy.md](session-proxy.md) — the `GraphController`: the engine-side
  mutation path and the single hook dispatch point. **Read it first**; this document
  assumes it. Consequences here: hooks fire from the controller; `el.Session`/`el.Graph`
  reach it per call through `Context` → services → `EngineService` (CLAUDE.md *Lua
  Bindings*); the model stays a data layer with no engine verbs. The `el.engine` facade
  module previously planned is dropped.

## Context

Nodes already carry scripts: a `scripts` child tree of `Script` models (name / type /
gzip'd code), with `Graph::findViewScript()` resolving the `View` script and `ScriptNode`
embedding DSP source in its state blob. The Session root does **not** have a scripts tree
(`src/session.cpp` — children are `graphs`, `controllers`, `maps`, `midiMappings`, `ui`).

The console exists (`src/ui/luaconsole.cpp`) and evaluates in the shared app state, but
its prelude never loads in dev builds, `el.command` is broken, and there is no way to
change graph topology from Lua. There is no hook/event registry of any kind.

## Design principles

1. **WordPress-inspired hooks, not a clone.** Two dispatch kinds:
   - **actions** — fire-and-forget notifications (`node.added`, `session.loaded`, ...)
   - **filters** — each handler receives a value and returns a (possibly modified) value;
     the host uses the final result.
   Handlers have an integer priority (default 10, lower runs first) and an owner tag for
   bulk teardown. Events are explicit, dispatch is message-thread only, and the C++ side
   owns the registry.

2. **Untrusted by default — capabilities are earned.** A session file is a *document*;
   documents that carry executable code must not silently get the keys to the app.
   Session scripts run in a restricted `sol::environment`; anything powerful must be
   declared in the descriptor (`requires = { 'session' }`) and granted by the host.

3. **Bind less C++, write more Lua.** C++ binds a minimal opaque handle; the ergonomic
   API is a native Lua module holding it. Precedent: `src/el/object.lua`, `command.lua`,
   `script.lua`. New surface (`el.hooks`) is Lua-first with a thin C core.

4. **One mutation path.** Every topology change — UI, undo, Lua, session load — goes
   through `GraphController`. Hooks are dispatched there and nowhere else for those
   events.

## What gets built

### 1. `HookBus` (`include/element/hooks.hpp`, `src/hooks.cpp`)

A plain registry; no `boost::signals2` groups needed.

```cpp
struct HookEvent {
    juce::String action;
    Node node;      // the node/graph the event is about (invalid if n/a)
    Node graph;     // its parent graph, or the root graph for graph.* events
    Arc arc;        // connection.* events
    int index = -1; // graph.moved / graph.activated
};

class HookBus {
public:
    using Action = std::function<void (const HookEvent&)>;
    using Filter = std::function<juce::var (juce::var, const HookEvent&)>;

    int  addAction (const juce::String& name, Action, int priority = 10, const juce::String& owner = {});
    int  addFilter (const juce::String& name, Filter, int priority = 10, const juce::String& owner = {});
    void remove (int id);
    void removeOwner (const juce::String& owner);

    void      doAction     (const juce::String& name, const HookEvent& = {});
    juce::var applyFilters (const juce::String& name, juce::var value, const HookEvent& = {});
    bool      hasActions   (const juce::String& name) const;

    struct ScopedSuspend { explicit ScopedSuspend (HookBus&); ~ScopedSuspend(); };

private:
    struct Handler { int id; int priority; juce::String owner; Action action; Filter filter; int errors = 0; bool enabled = true; };
    std::map<juce::String, std::vector<Handler>> actions, filters;
    int suspendCount = 0, depth = 0;
    std::deque<std::pair<juce::String, HookEvent>> pending;
};

namespace hooks {
constexpr const char* appStarted        = "app.started";
constexpr const char* appShutdown       = "app.shutdown";
constexpr const char* sessionLoaded     = "session.loaded";
constexpr const char* sessionSaving     = "session.saving";
constexpr const char* sessionClosed     = "session.closed";
constexpr const char* graphAdded        = "graph.added";
constexpr const char* graphRemoving     = "graph.removing";
constexpr const char* graphRemoved      = "graph.removed";
constexpr const char* graphMoved        = "graph.moved";
constexpr const char* graphActivated    = "graph.activated";
constexpr const char* nodeAdded         = "node.added";
constexpr const char* nodeRemoving      = "node.removing";
constexpr const char* nodeRemoved       = "node.removed";
constexpr const char* connectionAdded   = "connection.added";
constexpr const char* connectionRemoved = "connection.removed";
constexpr const char* graphDefaultName  = "graph.defaultName";   // filter
}
```

Rules:

- `doAction`/`applyFilters` assert the message thread. While `suspendCount > 0` they are
  no-ops.
- Dispatch iterates a *copy* of the handler list sorted by priority (handlers may add or
  remove during dispatch).
- Nested `doAction` (a handler mutating topology) is queued in `pending` and drained after
  the outer dispatch returns; `depth` is asserted `< 8`.
- Every handler call is wrapped; an exception increments `errors`, three consecutive
  errors disable the handler; a success resets the count. Errors go to
  `Context::logger()` (and, for Lua handlers, `ScriptingEngine::logError`). Nothing
  propagates.
- Filters thread the `juce::var`; a handler returning `void`/`nil` leaves the value
  unchanged.

Ownership: `Context::Impl` owns `std::unique_ptr<HookBus> hooks` with accessor
`HookBus& Context::hooks()`. Created in `init()` **before** `services` (so services
subscribe in `activate()`). In `freeAll()`
([context.cpp:94-106](../../src/context.cpp#L94)) reset **right after `services`** and
before `lua`: Lua-backed handlers hold `sol::protected_function`s and must die while the
state is alive.

Proof-of-shape filter: `graph.defaultName`, applied in `GraphController::addGraph (name)`
to the generated `"Graph N"` string. Single call site, no UI paint-path cost. Filters
like `node.displayName` are deferred until a call site that is not in a paint loop is
chosen.

### 2. Dispatch sites

See [session-proxy.md](session-proxy.md) § *Hook dispatch points*. Summary: every
`GraphController` mutation fires its action; `reload()` runs under `ScopedSuspend`;
`SessionService::notifySessionLoaded()` fires `session.loaded` (replacing the direct
`sigSessionLoaded()` at `pluginprocessor.cpp:800` too); `session.saving` beside
`sigWillSave`; `session.closed` in `closeSession()`; `app.started` at the end of
`Services::launch`; `app.shutdown` in `ScriptingService::deactivate()`.

`EngineService::sigNodeRemoved` is deleted; `GuiService` and `GraphEditorView` subscribe
to hooks instead (details in the controller doc).

### 3. Console foundation (`src/ui/luaconsole.*`, `luaconsoleview.*`, `src/scripting.*`)

- **Evaluator in the engine.** Implement the declared-but-missing
  `ScriptingEngine::execute` as
  `juce::Result execute (const juce::String& code, sol::environment env)`: the
  `"return <code>;"` compile probe, `sol::protected_function` with a `debug.traceback`
  handler, byte length from `toRawUTF8()`. Result message carries the traceback.
  Asserts the message thread (`JUCE_ASSERT_MESSAGE_THREAD`); the shared state has no
  other guard, and this is the entry point every later caller (console, hook scripts,
  extension entry scripts) goes through.
  `LuaConsole::textEntered` becomes a thin caller; drop the `_G.print` swap, the dead
  `lastError`/`errorHandler`, and the unreachable error branch.
- **Persistent environment.** `sol::environment& ScriptingEngine::consoleEnvironment()`,
  lazily created with globals fallback (`sol::environment (state, sol::create,
  state.globals())`). `LuaConsoleView::initializeView` uses it, so variables, `print`
  override and registered hooks survive view toggles. History moves alongside it
  (`ScriptingEngine::consoleHistory()` or keep it in the env as a Lua table).
- **Prelude from binary data.** `LuaConsole::setEnvironment` runs `scripts::console_lua`
  (`luascripts.hpp`, already compiled by `scripts/CMakeLists.txt`) in the env with
  `sol::script_pass_on_error` and reports failures to the console. `el.script.exec`
  is changed to `error()` when `load` fails instead of returning the message.
  `scripts/console.lua` gains:

  ```lua
  hooks   = require ('el.hooks')
  Context = require ('el.Context')
  session = function() return Context.instance():session() end   -- a function: never cache the session
  console.log = function (...) print (...) end                     -- to the console, not stdout
  ```

- **Threading.** `LuaConsoleView::messageLogged` posts to the message thread with
  `juce::MessageManager::callAsync` and a `Component::SafePointer`; `printMessages` is
  guarded by a `juce::CriticalSection` (or replaced with an `AsyncUpdater`).
- **`el.command`.** `el.Context` gains `commands()`, resolved per call through
  `ctx.services().find<GuiService>()->commands()`, so `command.lua`'s existing
  `Context.instance():commands()` is correct as written. No new globals: `el.context` is
  the single entry point into the app. `scripts/commands.lua` is deleted.
- Cosmetic: `EL_VIEW_CONSOLE` becomes `"LuaConsoleView"` (accept the old
  `"LuaConsoleViw"` when restoring `ContentContainer_lastSecondaryView`); `showConsole`
  gets a default keypress alongside `showPatchBay`/`showGraphEditor`.

### 4. Lua surface for mutation (`src/el/Session.cpp`, `Graph.cpp`, `nodetype.hpp`)

All of these resolve the `GraphController` per call through
`Context` → services → `EngineService` (see the controller doc § *Lua reaches the
controller through the services*) and therefore fire hooks. With no active engine they
return `nil, "engine not running"`; nothing falls back to silently editing the tree.

`el.Session`:

```lua
local s = session()
g = s:addGraph ("Rig" [, active])       -- string or el.Graph template; returns el.Graph
s:removeGraph (index)  s:moveGraph (from, to)  s:setActiveGraph (index)
s:activeGraph()  s.activeGraphIndex  s:findNodeById (uuid)
s[i]                                    -- now returns el.Graph, not el.Node
```

`el.Graph`:

```lua
n = g:addNode ("element.volume" [, format])    -- returns el.Node or nil, "message"
p = g:addPlugin { format = "VST3", name = "..." | id = "..." }
g:removeNode (n)
g:connect (srcId, srcPort, dstId, dstPort)  g:disconnect (...)
g:connectChannels (src, sc, dst, dc [, "midi"])
for node in g:nodes() do ... end   for arc in g:connections() do ... end
```

- `luaopen_el_Session` requires `el.Graph` (as `Context.cpp:60-63` requires `Node`/`Session`).
- In `nodetype.hpp` the index metamethod returns a `Graph` userdata when
  `child.isGraph()` via `sol::make_object`, else a `Node`, so the `Graph (const Node&)`
  type assert never fires for plain nodes.
- Lua two-value error convention: `nil, "message"`; never throws into the console.

### 5. `el.hooks` — Lua-first

Thin C core `src/el/Hooks.cpp` (`luaopen_el_Hooks`), registered in
`searchInternalModules` and `src/el/CMakeLists.txt`:

```
register (kind, name, fn, priority, owner) -> id     -- kind = "action" | "filter"
unregister (id)
unregister_owner (owner)
emit (name, event_table)
```

Wrapped by `src/el/hooks.lua`:

```lua
local hooks = require ('el.hooks')
local id = hooks.action ('node.added', function (e) print (e.node.name, e.graph.name) end [, priority])
hooks.filter ('graph.defaultName', function (name, e) return name .. ' *' end)
hooks.off (id)
hooks.emit ('my.event', { ... })                     -- custom events
local mine = hooks.owner ('my-script')               -- scoped table: same API, owner pre-filled
mine.action (...); mine.clear()
```

Handlers receive `{ action=, node=<el.Node|el.Graph>, graph=<el.Graph>, arc=, index= }`.
Filters receive `(value, event)` and return the value. The C side keeps a per-state map
`id → sol::protected_function` so `unregister` and state teardown can drop references.
Console registrations default to owner `"console"`; `ScriptingService` sets a "current
owner" while running a script so `hooks.action` picks it up without the caller passing
it.

### 6. Model: session-level scripts

- `Session` gains a `scripts` child tree, identical shape to `Node::getScriptsValueTree()`
  ([node.hpp:442](../../include/element/node.hpp#L442)). Add `Session::scripts()`,
  `addScript()`, `removeScript()` mirroring `Node::addScript`
  ([node.cpp:476](../../src/node.cpp#L476)).
- New script type tag `EL_TAG (Hook)` in `include/element/tags.hpp` (joins `DSP`,
  `View`, `GraphView`, `Anonymous`). `Script::make` ([script.cpp:183](../../src/script.cpp#L183))
  accepts `types::Hook` and seeds the template below.
- Additive child tree — no `EL_SESSION_VERSION` bump; add a no-op guard in
  `Session::migrate`.
- Persistence is free: `Script` code is already gzip'd into the tree.

### 7. Hook script descriptor (portable session ↔ extension)

```lua
--- Session hooks example.
-- @script  my-session-hooks
-- @type    Hook
return {
    type     = 'Hook',
    requires = { 'session' },          -- capabilities this script needs
    attach = function (hooks, ctx)     -- called once when the script is activated
        hooks.action ('session.loaded', function() ... end)
        hooks.action ('node.added', function (e) ... end, 20)   -- priority
        hooks.filter ('graph.defaultName', function (name, e)
            return name .. ' *'
        end)
        if ctx.session then ctx.session():addGraph ('Auto') end
    end,
    detach = function() ... end,       -- optional cleanup
}
```

`ctx` is the capability table: only granted entries are present. Scripts that got nothing
still get `hooks` — pure observers and cosmetic filters need no grants.

### 8. Restricted environment and capabilities

`ScriptingEngine::createRestrictedEnvironment()` returns a `sol::environment` with **no**
globals fallback, populated with:

- Base: `assert error ipairs next pairs pcall select tonumber tostring type unpack
  xpcall rawequal rawget rawset setmetatable getmetatable` and the tables `string`,
  `table`, `math`, `utf8`; `os` limited to `time clock date difftime`. No `io`, no
  `debug`, no `load`/`loadstring`/`dofile`/`loadfile`.
- `require` replaced by a function that only resolves an allowlist:
  `el.bytes`, `el.midi`, `el.strings`, `el.color`, `el.object`, `el.hooks`,
  `el.MidiMessage`, `el.MidiBuffer`, `el.AudioBuffer`.

Capabilities → `ctx` keys, granted per script from its `requires`:

| `requires` entry | `ctx` key | What it injects |
|---|---|---|
| `session` | `ctx.session` | function returning the live `el.Session` (carries mutation) |
| `engine` | `ctx.engine` | alias of `session` for readability; reserved for future engine-only calls |
| `ui` | `ctx.ui` | `el.Content` + `el.Commands.instance()` |
| `io` | `ctx.io` | the real `io` table |

The same restricted env (with the widget modules `el.View`, `el.Widget`, `el.Slider`,
`el.TextButton`, `el.Graphics`, `el.Bounds`… added to the allowlist) is applied in
`ScriptView::setScript` via `loader.call (env)`, closing the "View scripts run in raw
globals" hole.

**Compatibility.** View scripts embedded in existing user sessions run with full globals
today, so enforcing the allowlist is a behaviour change that can break a saved session
silently. Before flipping it: audit what the shipped and example View scripts
(`scripts/*.lua`, `docs/`, test snippets) actually `require` and which globals they
touch; then ship one release where `ScriptView` runs the script in the restricted env
with a *reporting* `require`/`__index` that logs each disallowed access to
`ScriptingEngine::logError` but still resolves it; enforce only after that log is quiet.
Anything a View script legitimately needs (e.g. `el.Context` read access) is added to
the allowlist rather than worked around.

### 9. Trust

- `Settings` gains `sessionScriptTrust`: map of *scripts-subtree SHA1* → `allow | deny`.
  Hashing only the `scripts` subtree invalidates trust on any edit and ignores unrelated
  session changes.
- On `session.loaded`, if any `Hook` script has a non-empty `requires` and the hash is
  unknown: asynchronous `AlertWindow` — "This session contains scripts that want: session
  access. **Run** / **Always for this session** / **Don't run**". *Run* grants for this
  process only; *Always* persists `allow`; *Don't run* persists `deny`. Scripts with
  empty `requires` attach immediately with no prompt.
- A "Re-ask trust" action clears the entry for the current hash.

### 10. Runner: `ScriptingService` (`src/services/scriptingservice.hpp/.cpp`)

New `Service`, registered in the `Services` ctor after `EngineService` and before
`SessionService`. Per the teardown rule, `activate()` captures `HookBus*`,
`ScriptingEngine*`, `SessionService*`, `Settings*` and never calls `context()` in the
destructor.

- `activate()`: subscribe `session.loaded` → `attachAll()`; `session.closed` → `detachAll()`;
  run `~/Music/Element/Scripts/init.lua` if present (owner `"user"`, globals-fallback env,
  no trust prompt — user-installed); fire `app.started` when `Services::launch` completes.
- `attachAll()`: for each `Hook` script in `session->scripts()`: restricted env →
  protected run → descriptor table → check `requires` against grants (prompt if needed)
  → `attach (hooks_for_owner, ctx)`. Owner id = the script's tree UUID.
- `detachAll()` / reload: call `detach` (protected), `HookBus::removeOwner (uuid)`, drop
  the env.
- `reattach (Script)`: called by `ScriptEditorView` after saving a hook script (detach →
  re-run), so edits take effect without reloading the session.
- `deactivate()`: `detachAll()`, fire `app.shutdown`, unsubscribe.

### 11. UI (minimal)

- Session panel (`src/ui/sessiontreepanel.cpp`, where node scripts are listed around
  `:507` and the `TODO enable script types at the graph level` at `:822`): list session
  scripts, add (`Script::make (types::Hook)`) / remove, open in the existing
  `ScriptEditorView`.
- Status line "scripts: N attached, M blocked" with the "Re-ask trust" action.

### 12. Dead-code cleanup (same PR series)

Delete: `ScriptingEngine::L`; `State::builtins` and `EL_LUA_SPATH`;
`Impl::scanDefaultLoctaion`; `scripts/commands.lua`;
the orphaned `test/snippets/sol3_parent.lua` and `stream_from_c.lua`. **Keep**
`addPackage`, `State::packages` and `resolve_internal_package`: the searcher is live
(position 3 of `package.searchers`, see the audit § 1) and [extensions.md](extensions.md)
Phase 1 registers extension modules through it; delete only `State::builtins` and the
commented-out `fill_builtins`. Register `el.vector` or delete `vector.c`. Fix the missing
comma in `widget.hpp` `__props`. Make `el/session.lua` resolve the session per call.
Restore the real body of `DSPScript::validate` (currently `#if 0`, returns `ok()` for
any non-empty string) or delete the method and its callers. Leave `ScriptManager` with a
comment that it is test-only until extensions land. Replace the hard-coded `== 13` in
`ScriptManagerTest` with a lower bound. **Keep** `ScriptInstance` and `DSPUIScript` (base and
placeholder for the script-type hierarchy: DSP, DSPUI, View, Hook…) and
`ScriptSource`/`ValueTreeScriptSource` (where a script's code comes from;
`ValueTreeScriptSource` is what session `Hook` scripts in §6 read their code through).

## Order of work

Each step builds and passes `ctest` before the next. Steps 1 and 2 depend on nothing in
[session-proxy.md](session-proxy.md) and land first: they fix what the audit found broken
and give a working REPL for exercising every later step by hand.

1. Dead-code cleanup (§12) + console foundation (§3) + `el.command` fix; `ConsoleTests`.
   Touches `src/scripting*`, `src/ui/luaconsole*`, `src/el/`, `scripts/` only — not
   `EngineService`.
2. `HookBus` + `HookBusTests` (no callers yet).
3. `GraphManager` stops showing alerts: failures return an invalid `Node`/`false`, the
   alerts move to `EngineService`. Prerequisite for headless controller tests.
4. The headless "real graphs through `attach` + `setRootNode`" test first (the untested
   path in [session-proxy.md](session-proxy.md) § Risks — a modal alert on that path
   hangs the runner, so prove it before moving code). Then `GraphController` +
   `EngineService` forwards in one PR so all callers keep compiling;
   `GuiService`/`GraphEditorView` hook handlers; `session.*` events;
   `GraphControllerTests`.
5. Lua surface: `el.Session`/`el.Graph` verbs, `el.hooks`; `SessionLuaTests`.
6. View-script `require` audit and the reporting pass (§8 *Compatibility*), then session
   `scripts` tree, `types::Hook`, restricted env, trust, `ScriptingService`, panel UI;
   `HookScriptTests`.

Format with `util/format.py`. Headers under `src/` first; `include/element/` only for
the public model API (`hooks.hpp`, `session.hpp`, `graph.hpp`, `tags.hpp`).

## Testing (Boost.Test)

Register every suite in `test/CMakeLists.txt` with
`add_test (NAME "<Suite>" COMMAND test_element --run_test=<Suite>)`.

- `HookBusTests` (`test/HookBusTests.cpp`): priority order; filter value threading;
  `remove`/`removeOwner`; auto-disable after 3 errors and re-enable after success;
  nested `doAction` queued not recursed; `ScopedSuspend`; off-thread `doAction` asserts.
- `GraphControllerTests` — see [session-proxy.md](session-proxy.md).
- `ConsoleTests` (`test/scripting/consoletests.cpp`): `execute ("1+1")` prints `2`;
  `execute ("x = 5")` then `execute ("x")` prints `5` in the persistent env; a syntax
  error yields a failed `Result` whose message contains a traceback line; prelude loads
  from binary data and defines `hooks`, `session`, `console`.
- `SessionLuaTests` (`test/scripting/sessionluatest.cpp`, snippet
  `test/snippets/session_mutate.lua`): state initialised with `test::context()`;
  `session():addGraph ('lua')`, `g:addNode ('element.volume')`, `hooks.action
  ('node.removed', …)`, `g:removeNode (n)` → handler ran once; `g:addNode ('bogus')`
  returns `nil, msg`; with services deactivated, `session():addGraph ('x')` returns
  `nil, "engine not running"` and the tree is unchanged.
- `HookScriptTests` (`test/scripting/hookscripttests.cpp`): session `scripts` tree
  round-trips through XML; descriptor with `requires = {'session'}` not granted →
  `attach` never called and no error spam; granted → `ctx.session` present and nothing
  else; restricted env has no `io`, `os.execute`, `load`, or `require ('el.Context')`;
  `detach` + `removeOwner` leave no handlers; `reattach` after an edit replaces them.
- Manual: View → Console; `session():addGraph ("Test")`;
  `hooks.action ('node.added', function (e) print (e.node.name) end)` then add a plugin
  from the UI → name printed; undo/redo the add → exactly one `node.removed` then one
  `node.added`; add a `Hook` script to the session, save, reopen → trust prompt, then
  `attach` runs.

## Relationship to the extensions plan

- `HookBus`, `el.hooks`, the restricted-env + capability machinery, `ScriptingService`
  and the mutation verbs all land here. [extensions.md](extensions.md) Phase 2 becomes
  "reuse the `el.Session`/`el.Graph` bindings"; Phase 3 reduces to wiring extension ownership and a
  friendlier default grant; extension entry scripts reuse the same descriptor and
  capability conventions.
- The capability model is also the answer to "extensions are trusted-but-isolated" —
  one mechanism, two default policies.

## Open questions

1. Trust key: content hash of the `scripts` subtree (chosen) vs file path. Revisit if the
   prompt is too frequent in practice.
2. Hook scripts on *graphs* (not just the session root): yes eventually, graph-scoped
   hooks firing only while that graph is active; session-root only in Phase 0.
3. Filter catalogue beyond `graph.defaultName`: candidates are `session.saving` payload
   and `node.displayName`, the latter only once a non-paint call site is identified.
4. Undoable Lua mutations: needs `AppMessage`-based routing or an explicit
   `UndoManager` transaction API on the controller. Out of scope for Phase 0.
5. Engine-initiated active-graph changes (MIDI program change,
   [audioengine.cpp:657-665](../../src/engine/audioengine.cpp#L657), writes
   `tags::active` from an async update) bypass the controller, so `graph.activated` does
   **not** fire for them in Phase 0. Document it as such in the hook catalogue. Fix is a
   `ValueTree::Listener` on the `graphs` child inside `GraphController` with a self-change
   flag; follow-up after step 4.
