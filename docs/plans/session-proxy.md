# Graph Controller — the engine-side mutation path

Architectural decision behind [session-scripts.md](session-scripts.md): where topology
changes to the live session are performed, and where hook events are dispatched from.
See [scripting-audit.md](scripting-audit.md) for the state of the code this builds on.

Formerly "SessionProxy". Renamed because the object is not a proxy for the session: it
is the engine-side controller of the session's graphs. The file name is kept so links
elsewhere stay valid.

## Motivation

Today `EngineService` is the only coordination point between model and engine, and it
keeps them aligned by ordering and by index:

- Root graphs: `EngineService::addGraph (const Node&, bool)`
  ([engineservice.cpp:347-386](../../src/services/engineservice.cpp#L347)) creates a
  `RootGraphHolder`, `attach (engine)`, *then* `session->addGraph (node, makeActive)`;
  `Session::addGraph` ([session.cpp:109](../../src/session.cpp#L109)) is a pure tree op.
- Nodes: `GraphManager::addNode` ([graphmanager.cpp:376-413](../../src/engine/graphmanager.cpp#L376))
  is engine-first: create the `Processor`, copy the tree, set `id`/`object`/`type`,
  `nodes.addChild`.
- `EngineService::moveGraph` calls `AudioEngine::moveGraph` then `Session::moveGraph`.
  That "pure model primitive + pure engine primitive, coordinated by the service" split
  (#1184) was done deliberately so a single coordinator could call both.

Two things are wanted from the change:

1. **One mutation path.** Every topology change — menu, undoable action, Lua, session
   load — goes through one object, so `graph.added` / `node.removed` /
   `connection.added` fire exactly once regardless of source. That object is the single
   `HookBus` dispatch point for those events.
2. **Separation of concerns.** The model (`Session`, `Graph`, `Node` in
   `include/element`) stays a data layer: value types over a `ValueTree`, no engine
   headers, callable from anywhere. The engine layer owns engine changes. The binding
   layer (`src/el/`), which already knows both, wires Lua to the controller. Dependencies
   point one way: engine → model, never model → engine.

## Options considered

**A — controller object stored in the tree** (a ref-counted object under a
`tags::proxy` property, the `tags::updater` idiom). Rejected on lifetime grounds:
`Context::Impl::freeAll()` frees `services` first and `session` later
([context.cpp:94-106](../../src/context.cpp#L94)); `PluginProcessor::~PluginProcessor`
deactivates services and *then* calls `session->clear()`; `Session::clear()` and
`loadData()` strip or replace the tree; a `var`-held object serializes as
`"Object 0x…"` unless stripped.

**B — leave `EngineService` as the facade.** Least change, but `RootGraphs` stays inside
the service, the hook dispatch is smeared across it, and it stays the god object.

**C — controller registered on the `Session`, discoverable from any `Node` copy via a
`Session::Handle` planted on the root tree, with engine verbs on the model.** Rejected
after review:

- It makes `include/element` depend on `src/engine`, makes value types
  message-thread-only, and lets a `Graph` copy instantiate plugins.
- The "detached copies are inert" claim does not hold. [session.cpp:170](../../src/session.cpp#L170),
  [:338](../../src/session.cpp#L338) and [:373](../../src/session.cpp#L373) copy the
  *root* tree, and a root copy carries the handle; `findFor` on that copy would resolve
  the live session and mutate the running engine.
- `Session::addGraph` behaving differently depending on whether a controller happens to
  be installed is a hidden mode.

**D — `EngineService`-owned controller, reached through the services.** Chosen. Same
object as C (it absorbs `RootGraphs` and is the hook dispatch point), but the model is
not involved: callers are `EngineService` forwards, the undo actions (unchanged, they
call `EngineService`), and the Lua bindings, which resolve it per call through
`Context` → services → `EngineService`, per the *Lua Bindings* rules in CLAUDE.md.

## Design

### Ownership and lifetime

- `GraphController` (`src/engine/graphcontroller.hpp/.cpp`) becomes the home of
  `RootGraphHolder` and `RootGraphs`, moved verbatim from
  [engineservice.cpp:78-305](../../src/services/engineservice.cpp#L78). The constructor
  takes `Context&`, `AudioEnginePtr`, `SessionPtr` and `HookBus&` (injected, never
  looked up). The destructor never calls `context()` (services may already be gone).
- `EngineService` owns `std::unique_ptr<GraphController> controller`: created in
  `activate()` after `engine->activate()` and before `sessionReloaded()`; reset in
  `deactivate()` after `session->saveGraphState()` (replaces `graphs->clear()`).
  `GraphController* EngineService::controller()` returns null when inactive.
- The controller survives session reload for free: `loadData()`/`clear()` touch the
  tree, not the controller. Holders are rebuilt by `sessionReloaded()` → `reload()`.
- The model is untouched: no `Session::Handle`, no `findFor`, no weak pointer in
  `Session::Impl`, no engine verbs on `Session`/`Graph`. `Session::addGraph`,
  `moveGraph`, `setActiveGraph` stay what they are today: pure tree operations.

### `GraphController` API

All methods return `bool`/`Node`, never show UI, and assert the message thread.

```cpp
// root graphs
bool addGraph (const Node& graph, bool makeActive);
Node addGraph (const juce::String& name);          // default ports from engine->getNumChannels
bool removeGraph (int index);
bool moveGraph (const Node& graph, int newIndex);
bool setActiveGraph (int index);                   // today's setRootNode + Session::setActiveGraph
void reload();                                     // today's sessionReloaded, under HookBus::ScopedSuspend
void clear();  void syncModels();
// nodes
Node addNode (const Node& graph, const Node& nodeTemplate, const ConnectionBuilder& = {});
Node addPlugin (const Node& graph, const juce::PluginDescription&, double rx = .5, double ry = .5);
bool removeNode (const Node&);                     // incl. the IO-port workaround at engineservice.cpp:717-738
Node replace (const Node&, const juce::PluginDescription&);
bool changeBusesLayout (const Node&, const juce::AudioProcessor::BusesLayout&);
// connections
bool addConnection (const Node& graph, uint32 s, uint32 sp, uint32 d, uint32 dp);
bool removeConnection (const Node& graph, uint32 s, uint32 sp, uint32 d, uint32 dp);
bool connectChannels (const Node& graph, const Node& src, int sc, const Node& dst, int dc,
                      PortType = PortType::Audio, int count = 1);
struct DisconnectOptions { bool inputs = true, outputs = true, audio = true, midi = true; };
void disconnectNode (const Node&, DisconnectOptions = {});   // per-arc removeConnection
// lookup
GraphManager* findGraphManagerFor (const Node&) const;       // by graph tree identity
RootGraphManager* findActiveRootGraphManager() const;
```

- `addNode`/`addPlugin` pre-validate through `PluginManager::findDescriptionFor` /
  `getKnownPlugins()` so a bad identifier from Lua returns an invalid `Node`.
- `DisconnectOptions` replaces today's four positional booleans; no new method on the
  controller takes more than one `bool`.
- `findGraphManagerFor` matches on the graph's underlying tree, so any `Node` copy of a
  live graph resolves; a copy of a detached subtree resolves to null and the call fails.

### `GraphManager` never shows UI

`GraphManager` shows modal alerts at
[graphmanager.cpp:29-31](../../src/engine/graphmanager.cpp#L29), [:380](../../src/engine/graphmanager.cpp#L380),
[:408](../../src/engine/graphmanager.cpp#L408) and [:418](../../src/engine/graphmanager.cpp#L418).
Pre-validation in the controller catches "unknown plugin" but not "found but failed to
instantiate", and a modal in that path hangs a headless test. `GraphManager::addNode`
and friends return an invalid `Node`/`false` and log; the alert moves to
`EngineService`, which is where UI policy lives after this change. This is a
prerequisite for the controller and lands as its own step (see *Order of work* in
session-scripts.md).

### `EngineService` afterwards

Every public signature in [engine.hpp](../../include/element/engine.hpp) stays, as a
thin forward to the controller plus the UI/plugin-list policy that does not belong in
the controller: the alerts (including those moved out of `GraphManager`),
`presentPluginWindow` (only on the entry points that show it today, so undo/redo never
pops windows), `detail::verifyPlugin` / `saveUserPlugins` / `addToKnownPlugins`,
`addMidiDeviceNode` (rewritten over `controller->addPlugin` + `Node::getObject()`),
`stabilizeViews` after `replace`/`changeBusesLayout`. Group the alert and plugin-list
policy in a `detail::` helper inside `engineservice.cpp` so the service itself is
forwards plus policy calls.

Callers that keep compiling unchanged: `src/messages.cpp` undo actions,
`src/services.cpp:224-320`, `guiservice.cpp:1070-1073`,
`sessiontreepanel.cpp:592/693/1079`, `sessionservice.cpp:104`.

The active-graph-only overloads (`addConnection (s, sp, d, dp)`, `removeNode (uint32)`)
resolve `session->getActiveGraph()` and forward — this also fixes `removeNode (uint32)`
([engineservice.cpp:755-763](../../src/services/engineservice.cpp#L755)) bypassing
notification today.

Moves out of `EngineService` entirely: `RootGraphHolder`, `RootGraphs`, the bodies of
`addGraph (Node, bool)`, `removeGraph`, `moveGraph`, `setRootNode`, `sessionReloaded`,
`syncModels`, `addNode (Node, Node, Builder)`, private `addPlugin (GraphManager&, …)`,
`removeNode (Node)`, the graph overloads of `addConnection`/`removeConnection`,
`connectChannels`/`connect`, `disconnectNode`, `replace`, `changeBusesLayout`, `clear`.

`sigNodeRemoved` is deleted. Its consumer
([grapheditorview.cpp:80,158](../../src/ui/grapheditorview.cpp#L80)) subscribes to
`graph.removed` on `context().hooks()` instead.

### Lua reaches the controller through the services

Per CLAUDE.md *Lua Bindings*: one entry point, resolve per call, no globals. The
`el.Session`/`el.Graph` methods in [session-scripts.md](session-scripts.md) § 4 are
implemented in `src/el/Session.cpp` / `Graph.cpp` as:

```cpp
static GraphController* controllerFor (lua_State* L)
{
    auto& ctx = element::lua::contextFrom (L);          // reads _G["el.context"] like el.Context.instance()
    auto* engine = ctx.services().find<EngineService>();
    return engine != nullptr ? engine->controller() : nullptr;
}
// el.Graph:addNode
"addNode", [] (Graph& self, const std::string& id, sol::optional<std::string> format, sol::this_state L)
    -> std::tuple<sol::object, sol::object> {
    auto* c = controllerFor (L);
    if (c == nullptr) return { nil, "engine not running" };
    auto node = c->addNode (self, Node::makeTemplate (id, format...));
    return node.isValid() ? { make_object (node), nil } : { nil, "could not add " + id };
}
```

Nothing is cached: no controller pointer in a userdata, no session cached at require
time. A `Graph` userdata is only a tree handle; the controller decides whether it
refers to a live graph.

### Undo

No structural change. `AddPluginAction`, `RemoveNodeAction`, `AddConnectionAction`,
`RemoveConnectionAction` ([messages.cpp](../../src/messages.cpp)) keep calling
`EngineService`; each `perform()`/`undo()` is exactly one controller mutation and
therefore exactly one hook. Lua-initiated mutations are **not** undoable in this phase:
they bypass `GuiService::handleMessage`
([guiservice.cpp:1235-1245](../../src/services/guiservice.cpp#L1235)). Making them
undoable later means posting `AppMessage`s, which is asynchronous and cannot return the
created node — a deliberate non-goal for now.

### GUI reacts through hooks

- `GuiService::activate()` registers handlers (ids stored, removed in `deactivate()`):
  `node.removing` → `closePluginWindowsFor (node, true)` + deselect (moved from
  [engineservice.cpp:706-712](../../src/services/engineservice.cpp#L706));
  `graph.removed` → `stabilizeContent()` (replaces the `// FIXME: dont notify the UI
  top-down` at `:489`); `graph.activated` → close/show plugin windows (moved from
  [sessiontreepanel.cpp:618-627](../../src/ui/sessiontreepanel.cpp#L618) and
  [guiservice.cpp:171-175](../../src/services/guiservice.cpp#L171)).
- Direct writes of `tags::active` in `sessiontreepanel.cpp:624-626`, `:758` and
  `guiservice.cpp:173-174` become `EngineService::setActiveGraph (index)`; otherwise
  Lua-initiated activation and hooks diverge from the UI path.

### Hook dispatch points

Only `GraphController` fires topology actions: `graph.added`, `graph.removing`,
`graph.removed`, `graph.moved`, `graph.activated`, `node.added`, `node.removing`,
`node.removed`, `connection.added`, `connection.removed`. Pre-hooks (`*.removing`) exist
so the GUI can close plugin windows *before* the processor dies regardless of who
initiated the removal.

- `reload()` runs under `HookBus::ScopedSuspend`, so session load fires nothing per node
  (`GraphManager::setNodeModel` and `IONodeEnforcer` operate below the controller
  anyway) and the internal `setActiveGraph` during load does not leak `graph.activated`.
- `session.loaded` is fired once from a new `SessionService::notifySessionLoaded()`,
  which also emits `sigSessionLoaded()`; called from `refreshOtherControllers()` and
  from [pluginprocessor.cpp:800](../../src/pluginprocessor.cpp#L800) in place of the
  direct emit. Not fired from `reload()`: `EngineService::sessionReloaded()` is also
  called by `PluginProcessor::reloadEngine()` on every host re-prepare.
- `session.saving` next to `sigWillSave`; `session.closed` in `closeSession()`.
- `Session::ScopedFrozenLock` is left alone: it guards the document-dirty
  `ChangeBroadcaster`, a different concern, and `Services::run()` keeps the whole
  default-session open frozen, which would wrongly swallow a legitimate `graph.added`
  from an `.elg` import.

**Coverage gaps, decided:**

- `ConnectionBuilder` connections made while adding a node currently happen inside
  `GraphManager::addNode` ([graphmanager.cpp:801](../../src/engine/graphmanager.cpp#L801)),
  below the controller. The controller applies the builder itself through its own
  `addConnection` after the node exists, so each auto-connection fires
  `connection.added`; `GraphManager::addNode` loses its builder parameter.
- `Processor` re-applies channel connections after a bus-layout change at
  [processor.cpp:972-977](../../src/engine/processor.cpp#L972), also below the
  controller. Phase 0 fires no `connection.*` for these; `changeBusesLayout` is
  documented as "ports and their connections may change" and a `node.portsChanged`
  action is a candidate follow-up.
- Engine-initiated active-graph changes (MIDI program change,
  [audioengine.cpp:657-665](../../src/engine/audioengine.cpp#L657)) bypass the
  controller; `graph.activated` excludes them in Phase 0. Tracked as
  session-scripts.md open question 5.

## Future: model-initiated mutation, if ever wanted

If a later phase wants `Session::addGraph (...)` to drive the engine, the path is a
delegate interface **declared by the model and implemented by the controller** — not a
handle in the tree, and not engine headers in `include/element`:

```cpp
// include/element/session.hpp
class Session {
public:
    struct Delegate {
        virtual ~Delegate() = default;
        virtual bool addGraph (Session&, const Node& graph, bool makeActive) = 0;
        virtual bool removeGraph (Session&, int index) = 0;
        // one method per controller verb that should be model-initiated
    };
    void setDelegate (Delegate*);     // EngineService installs in activate(), clears in deactivate()
};
```

`Session::addGraph` calls the delegate synchronously: the controller attaches the
engine side first, then performs the tree op, then fires the hook, and the result is
returned to the caller. That is the ordering `EngineService::addGraph` already uses
today. Graph-level verbs take the graph as an argument on the session
(`session->addNode (graph, template)`) so `Graph`/`Node` copies stay inert. A
"model first, engine follows via `ValueTree::Listener`" design is explicitly not the
path: the caller cannot learn about engine failure, and reload/undo/`loadData` all have
to suspend the listener.

## Tests

`test/GraphControllerTests.cpp`, suite `GraphControllerTests`, using
`element::test::context()` ([TestMain.cpp:15-23](../../test/TestMain.cpp#L15), which
activates services so the controller exists) and a test node provider extracted from
`CountingNodeProvider` ([SessionLoadBenchTests.cpp:58-82](../../test/SessionLoadBenchTests.cpp#L58))
into `test/fixture/TestNodeProvider.h`:

- `ControllerInstalledAndCleared` — own `Context`, `services().activate()/deactivate()`,
  `controller()` non-null only while active.
- `AddGraphAttachesEngine` — `controller->addGraph` → `getObject()` non-null,
  `graph.added` once; the session tree gained the child.
- `AddNodeCreatesProcessor`, `ConnectDisconnect` (arc present in `arcs`, hooks fired).
- `BuilderConnectionsFireHooks` — `addNode` with a `ConnectionBuilder` → one
  `connection.added` per built arc.
- `RemoveNodeOrdering` — `node.removing` before `node.removed`.
- `RemoveGraphFixesActive`, `SetActiveGraphSyncsEngine` (`engine->getActiveGraphIndex()`).
- `ReloadFiresNoPerNodeHooks` — `loadData` + `sessionReloaded()` → zero `node.added`.
- `InstantiationFailureIsSilent` — a node type whose provider returns null → invalid
  `Node`, no modal, test does not hang (proves the `GraphManager` alert move).
- `DetachedGraphIsRejected` — `addNode` on a `Graph` copied out of the session → invalid
  `Node`, no hook.

Register with `add_test (NAME "GraphControllerTests" COMMAND test_element --run_test=GraphControllerTests)`.
Existing `SessionTests` keep covering the pure-tree path unchanged. Remember: a
`GraphNode` driven by `GraphManager` must be heap-allocated (`ProcessorPtr keep (new GraphNode (ctx))`).

## Risks / not verified

- Headless attach with real graphs: `SessionTests::MoveEngineGraph` proves
  `AudioEngine::addGraph` works without a device and `test::context()` runs
  `sessionReloaded()` with zero graphs, but the full `attach` + `setRootNode`
  (`setPlayConfigFor (devices)`) path with real graphs has no current test; sample
  rate/block size may be 0 headless. Write this test before moving code.
- `PluginProcessor::reloadEngine` → `reload()` must keep the full detach/re-attach
  behaviour; `prepareExternalPlayback` re-preparation was not traced.
- Re-entrant hooks (a handler adding a node from `node.added`) work through the depth
  guard but interleave `GraphManager::changed()` broadcasts. Untested territory.
- `RootGraphHolder` touches `Processor` internals as `friend class EngineService`
  ([processor.hpp:539](../../include/element/processor.hpp#L539)); `GraphController`
  needs adding to the friend list.
- `new_nodetype<Graph>`'s index metamethod already returns non-graph children as
  `Graph` today; the `Graph (const Node&)` constructor asserts type == Graph in debug
  builds. The `sol::make_object` split in session-scripts.md is required, not optional.
- The #1184 issue text was not read; the split-primitive intent is inferred from the
  `Session::moveGraph` doc comment and `EngineService::moveGraph`.
