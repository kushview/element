# Phase 0 — Session Scripts & the Hook System

Predecessor to [extensions.md](extensions.md). Front-loads the pieces the extension format
needs anyway — the hook bus, the script descriptor conventions, and the restricted
environment / capability model — but delivers them first for scripts **embedded in the
session file**, where DSP/DSPUI scripts already live today.

## Context

Nodes already carry scripts: a `scripts` child tree of `Script` models (name / type /
gzip'd code), with `Graph::findViewScript()` resolving the `View` script and `ScriptNode`
embedding DSP source in its state blob. The Session root does **not** have a scripts tree
(`src/session.cpp` — children are `graphs`, `controllers`, `maps`, `midiMappings`, `ui`).

Phase 0 extends the same pattern to the session: sessions can carry scripts that *run* at
defined lifecycle points. Scripts follow the **same descriptor format planned for
extensions**, so a script is portable between "written into the session" and "shipped in
a `.element` extension" without edits.

## Design principles

1. **WordPress-inspired hooks, not a clone.** Two dispatch kinds:
   - **actions** — fire-and-forget notifications (`node.added`, `session.loaded`, ...)
   - **filters** — each handler receives a value and returns a (possibly modified) value;
     the host uses the final result (e.g. filter a display name, a save payload, a menu).
   Handlers have an optional integer priority (default 10, lower runs first) and an owner
   tag for bulk teardown. No WordPress-style global mutable everything — events are
   explicit, dispatch is message-thread only, and the C++ side owns the registry.

2. **Untrusted by default — capabilities are earned.** A session file is a *document*;
   documents that carry executable code must not silently get the keys to the app. Session
   scripts run in a restricted `sol::environment`:
   - Base env: safe stdlib subset (no `io`, no `os` beyond `time`/`clock`, no `require`
     of arbitrary modules), plus pure-data `el.*` modules (bytes, midi, colors, strings).
   - Anything powerful — `el.engine`, `el.Context`, `el.ui`, file access — must be
     declared in the script descriptor (`requires = { "engine", "ui" }`) and granted by
     the host. Grant policy v1: per-session trust prompt on first run ("This session
     contains scripts that want: engine access. Run / Run always for this session /
     Don't run"), persisted in settings keyed by a session content hash or path.
   - Extensions (user-installed) get a more permissive default later; the *mechanism*
     (declared requires → injected capabilities) is identical.

3. **Bind less C++, write more Lua.** The rule going forward: C++ binds a minimal opaque
   userdata handle ("impl"), and the ergonomic API is a native Lua module holding that
   handle as a private member. Precedent already in-tree: `src/el/object.lua`,
   `session.lua`, `command.lua`, `script.lua` wrap C bindings in Lua tables. New surface
   (`el.hooks`, later `el.engine` sugar) should be Lua-first with a thin C core — this
   avoids sol2 usertype boilerplate for every class and keeps the public script API
   decoupled from C++ headers.

## What gets built

### 1. Model: session-level scripts

- `Session` gains a `scripts` child tree, identical shape to `Node::getScriptsValueTree()`
  (`include/element/node.hpp:434`). Add `Session::scripts()` / `addScript()` /
  `removeScript()` accessors mirroring `Node::addScript` (`src/node.cpp:476`).
- New script type tag: `EL_TAG(Hook)` in `include/element/tags.hpp` (joins `DSP`, `View`,
  `GraphView`, `Anonymous`). `Script::make` (`src/script.cpp:183`) accepts `types::Hook`
  and seeds a template.
- Additive child tree — no `EL_SESSION_VERSION` bump; add a no-op guard in
  `Session::migrate`.
- Persistence is free: `Script` code is already gzip'd into the tree, so `.els` stays a
  single file.

### 2. Hook script descriptor (portable session ↔ extension)

```lua
--- Session hooks example.
-- @script  my-session-hooks
-- @type    Hook
return {
    type     = 'Hook',
    requires = { 'engine' },          -- capabilities this script needs
    attach = function (hooks, ctx)    -- called once when the script is activated
        hooks.action ('session.loaded', function() ... end)
        hooks.action ('node.added', function (node) ... end, 20)  -- priority
        hooks.filter ('node.displayName', function (name, node)
            return name .. ' *'
        end)
    end,
    detach = function() ... end,      -- optional cleanup
}
```

`ctx` is the capability table: only granted entries are present (`ctx.engine`,
`ctx.context`, ...). Scripts that got nothing still get `hooks` — pure observers that,
e.g., filter cosmetic values, need no grants at all.

### 3. `HookBus` (C++, `src/scripting/hookbus.hpp/.cpp`)

Same class the extensions plan specifies (its Phase 3 shrinks to "wire more signals"):

- `addAction (event, sol::protected_function, priority, owner)` / `addFilter (...)` →
  `Handle`; `remove (Handle)`; `removeOwner (ownerId)`.
- `dispatchAction (event, pusher)` and `applyFilters (event, initialValue, pusher)`.
- Message-thread only (assert); dispatch iterates a copy (handlers may add/remove);
  reentrancy guard — nested dispatch queues and drains after; a handler is auto-disabled
  after 3 consecutive errors; every call is protected, errors go to
  `ScriptingEngine::logError`, never propagate.

Initial events (actions): `session.loaded`, `session.saving`, `graph.added`,
`graph.removed`, `graph.changed`, `node.added`, `node.removed`. Initial filters: start
with one or two proving the shape (e.g. `node.displayName`) — filters are the part to
grow cautiously. Signal sources: `SessionService::sigSessionLoaded` / `sigWillSave`
(`src/services/sessionservice.hpp:37-38`), `EngineService::sigNodeRemoved`, plus new
`sigNodeAdded` / `sigGraphAdded` / `sigGraphRemoved` emitted at the same sites in
`src/services/engineservice.cpp`.

### 4. `el.hooks` Lua module — Lua-first

Per principle 3: a small C binding exposing an opaque bus handle + `register/unregister/
emit` primitives (`src/el/Hooks.cpp`), wrapped by `src/el/hooks.lua` providing the
friendly `action`/`filter`/`off` API, priority defaults, and owner scoping. The `hooks`
object passed to `attach` is a Lua table closing over the owner id — no usertype needed.

### 5. Runner & lifecycle (`ScriptingService`, new — shared with extensions plan)

- On `sigSessionLoaded`: read the session `scripts` tree, for each `Hook` script:
  restricted env → run → descriptor table → check `requires` vs grants → prompt if
  needed → call `attach (hooks, ctx)`. Owner id = script's tree UUID.
- On session unload/reload/close: call `detach` (protected), `HookBus::removeOwner`,
  drop envs. Also handles in-session edits: saving a hook script in the editor
  re-attaches it (detach → re-run).
- `app.*` events and extension ownership come later (extensions plan); the bus API is
  already owner-based so nothing changes shape.

### 6. UI (minimal)

- Session properties / session panel: list session scripts, add/remove, open in the
  existing `ScriptEditorView` (`src/ui/scripteditorview.cpp`).
- A "scripts blocked / granted" indicator with a way to re-open the trust prompt.

## Testing (Boost.Test, `test/scripting/`)

- Session scripts tree round-trip (add → save XML → load → scripts intact).
- HookBus: priority order, filter value threading, auto-disable after errors,
  removeOwner, reentrancy (action handler emitting another action queues, no recursion).
- Runner: descriptor with `requires` not granted → `attach` never called, no error spam;
  granted → ctx contains exactly the granted capabilities.
- Restricted env: `io`/`os.execute`/raw `require` unavailable inside a hook script.
- Register suites in `test/CMakeLists.txt` with `add_test`.

## Relationship to the extensions plan

- HookBus, `el.hooks`, restricted-env + capability machinery, and `ScriptingService`
  move **here** (Phase 0). [extensions.md](extensions.md) Phase 3 reduces to wiring
  `app.started`/`app.shutdown` and extension-owned registration; extension entry scripts
  reuse the same descriptor/capability conventions with a friendlier default grant.
- The capability model is also the answer to "extensions are trusted-but-isolated" —
  one mechanism, two default policies.

## Open questions

1. Trust persistence key: content hash (safer, invalidates on edit) vs file path
   (friendlier). Suggest hash of the scripts subtree only.
2. Do hook scripts on *nodes/graphs* (not just the session root) run too? Suggest: yes
   eventually (graph-scoped hooks fire only while that graph is active), but session-root
   only in Phase 0.
3. Filter catalog — which host values are worth filtering first.
