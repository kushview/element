# Session Proxy — model-initiated engine changes

Architectural decision behind [session-scripts.md](session-scripts.md): how Lua (and
anything else) mutates the live session, and where hook events are dispatched from.
See [scripting-audit.md](scripting-audit.md) for the state of the code this builds on.

## Motivation

Direction: any modification to engine-layer state is **initiated in the model**
(`Session`, `Graph`, `Node`). Model objects hold a shared internal object — the
*proxy* — that performs the engine-side work and syncs state back. This is explicitly
not "the model emits a signal and hopes a service reacts".

Today `EngineService` is the only coordination point and it keeps model and engine
aligned by ordering and by index:

- Root graphs: `EngineService::addGraph (const Node&, bool)`
  ([engineservice.cpp:347-386](../../src/services/engineservice.cpp#L347)) creates a
  `RootGraphHolder`, `attach (engine)`, *then* `session->addGraph (node, makeActive)`;
  `Session::addGraph` ([session.cpp:109](../../src/session.cpp#L109)) is a pure tree op.
- Nodes: `GraphManager::addNode` ([graphmanager.cpp:376-413](../../src/engine/graphmanager.cpp#L376))
  is engine-first: create the `Processor`, copy the tree, set `id`/`object`/`type`,
  `nodes.addChild`.
- `EngineService::moveGraph` calls `AudioEngine::moveGraph` then `Session::moveGraph`.
  That "pure model primitive + pure engine primitive, coordinated by the service" split
  (#1184) was done deliberately so a proxy could call both.

The proxy is also the natural **single dispatch point for hooks**: if every topology
change passes through it, `graph.added` / `node.removed` / `connection.added` fire once
regardless of source (menu, undoable action, Lua, session load).

## Options considered

**A — proxy object stored in the tree** (a ref-counted object under a `tags::proxy`
property, the `tags::updater` idiom). Rejected on lifetime grounds:

- `Context::Impl::freeAll()` frees `services` first and `session` later
  ([context.cpp:94-106](../../src/context.cpp#L94)); `PluginProcessor::~PluginProcessor`
  deactivates services and *then* calls `session->clear()`
  ([pluginprocessor.cpp:373-390](../../src/pluginprocessor.cpp#L373)). A proxy living in
  the tree would hold pointers into a dead `EngineService`/`RootGraphs` unless it is
  disarmed on every deactivate path.
- `Session::clear()` (`setMissingProperties (true)`) and `Session::loadData()` (whole-tree
  swap, [session.cpp:150-160](../../src/session.cpp#L150)) strip or replace the tree.
- A `var`-held object serializes as `"Object 0x…"` unless added to
  `Node::sanitizeProperties`.
- It moves all of `RootGraphs` into an object whose lifetime is governed by `ValueTree`
  refcounts.

**B — service facade** (a `GraphService`/`EngineService` vocabulary plus an `el.engine`
Lua module resolving the service per call). Least change, but the models stay inert,
`EngineService` stays the god object, and model-initiated mutation is not achieved.

**C — service-owned proxy, registered on the Session, discoverable from the tree.**
Chosen. Lifetime follows the service; the model holds only a `weak_ptr`; a tiny handle
in the tree lets any `Node` copy find its `Session`.

## Design

### Ownership and lifetime

- `SessionProxy` (`src/engine/sessionproxy.hpp/.cpp`) becomes the home of
  `RootGraphHolder` and `RootGraphs`, moved verbatim from
  [engineservice.cpp:78-305](../../src/services/engineservice.cpp#L78). The constructor
  captures `Context&`, `AudioEnginePtr`, `SessionPtr` and `HookBus&`. The destructor
  never calls `context()` (services may already be gone).
- `EngineService` owns `std::shared_ptr<SessionProxy> proxy`: created in `activate()`
  after `engine->activate()` and before `sessionReloaded()`, followed by
  `session->setProxy (proxy)`; in `deactivate()` after `session->saveGraphState()`:
  `session->setProxy (nullptr)` then `proxy.reset()` (replaces `graphs->clear()`).
- `Session::Impl` ([session.cpp:78-90](../../src/session.cpp#L78), currently empty) holds
  `std::weak_ptr<SessionProxy> proxy`. Mis-ordered teardown degrades to "no proxy →
  pure tree op", never a dangling call.
- The proxy survives session reload for free: `loadData()`/`clear()` touch
  `objectData`, not `Impl`. Holders are rebuilt by `sessionReloaded()` → `proxy->reload()`.

### How a `Node` copy finds the proxy

`Node` is a value type; copies share only the tree. So the tree must lead back to the
`Session`:

- `Session::Handle : juce::ReferenceCountedObject { Session* session; }` is planted on
  the session root tree under a new `tags::sessionObject` — the same idiom as
  `GraphManager` planting `NodeModelUpdater` under `tags::updater`
  ([graphmanager.cpp:651-668](../../src/engine/graphmanager.cpp#L651)).
- `static SessionPtr Session::findFor (const juce::ValueTree& any)` reads
  `any.getRoot()[tags::sessionObject]` → handle → `Session*` → `SessionPtr`.
- Plant: `Session` ctor, `loadData`, `setMissingProperties (reset = true)`.
  Null: `~Session()` before `clear()`.
  Ignore: `Session::valueTreePropertyChanged`
  ([session.cpp:262](../../src/session.cpp#L262), alongside `object`/`updater`).
  Strip: `Node::sanitizeProperties` ([node.cpp:349](../../src/node.cpp#L349)), which
  `createXml`/`writeToFile` already call on the session copy.
- Detached copies (`.elg` export, `RemoveNodeAction::nodeData`, `createCopy()` of a
  graph subtree) carry no root handle → no proxy → pure tree ops. That is exactly the
  headless/undo-friendly behaviour wanted.

### `SessionProxy` API

All methods return `bool`/`Node`, never show an `AlertWindow`, and assert the message
thread.

```cpp
// root graphs
bool addGraph (const Node& graph, bool makeActive);
Node addGraph (const juce::String& name);          // default ports from engine->getNumChannels
bool removeGraph (int index);
bool moveGraph (const Node& graph, int newIndex);
bool setActiveGraph (int index);                   // today's setRootNode + Session::setActiveGraphData
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
void disconnectNode (const Node&, bool inputs, bool outputs, bool audio, bool midi); // per-arc removeConnection
// lookup
GraphManager* findGraphManagerFor (const Node&) const;
RootGraphManager* findActiveRootGraphManager() const;
```

`addNode`/`addPlugin` pre-validate through `PluginManager::findDescriptionFor` /
`getKnownPlugins()` so a bad identifier from Lua returns an invalid `Node` instead of
reaching the modal `AlertWindow` inside `GraphManager::addNode`
([graphmanager.cpp:380,408](../../src/engine/graphmanager.cpp#L380)).

### Model verbs

`Session` (`include/element/session.hpp`, `src/session.cpp`):

- `setProxy (std::shared_ptr<SessionProxy>)`, `proxy()`, `static findFor (ValueTree)`.
- Proxy-aware, existing names keep working: `addGraph (const Node&, bool)`,
  `moveGraph (int, int)`, `setActiveGraph (int)`; new `removeGraph (int)`,
  `Node addGraph (const juce::String& name)`. Each is
  `if (auto p = proxy()) return p->X (...); return XData (...);`.
- Pure primitives stay public, named like `loadData`: `addGraphData`, `removeGraphData`,
  `moveGraphData`, `setActiveGraphData`.
- `SessionService::loadNewSessionData`
  ([sessionservice.cpp:316](../../src/services/sessionservice.cpp#L316)) switches to
  `addGraphData`; with a proxy installed at that point the default graph would attach
  immediately and then be detached/re-attached by `refreshOtherControllers()`.

`Graph` (`include/element/graph.hpp`, `src/graph.cpp`):

- `Node addNode (const Node& nodeTemplate)`, `Node addNode (const juce::String& id, const juce::String& format = EL_NODE_FORMAT_NAME)`,
  `Node addPlugin (const juce::PluginDescription&)`, `bool removeNode (const Node&)`,
  `bool connect (uint32, uint32, uint32, uint32)`, `bool disconnect (…)`,
  `bool connectChannels (const Node& src, int sc, const Node& dst, int dc, PortType = Audio)`.
- Fallback without a proxy: copy the template into `nodes` with ids reset (what
  `SessionLoadBenchTests::makeGraphModel` does by hand), append/remove an `Arc` in
  `arcs`.
- `graph.cpp` includes `<element/session.hpp>` and `"engine/sessionproxy.hpp"`;
  `graph.hpp` forward-declares only.

### `EngineService` afterwards

Every public signature in [engine.hpp](../../include/element/engine.hpp) stays, as a
thin forward to the proxy plus the UI/plugin-list policy that does not belong in the
proxy: `AlertWindow` messages, `presentPluginWindow` (only on the entry points that show
it today, so undo/redo never pops windows), `detail::verifyPlugin` / `saveUserPlugins` /
`addToKnownPlugins`, `addMidiDeviceNode` (rewritten over `proxy->addPlugin` +
`Node::getObject()`), `stabilizeViews` after `replace`/`changeBusesLayout`.

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

### Undo

No structural change. `AddPluginAction`, `RemoveNodeAction`, `AddConnectionAction`,
`RemoveConnectionAction` ([messages.cpp](../../src/messages.cpp)) keep calling
`EngineService`; each `perform()`/`undo()` is exactly one proxy mutation and therefore
exactly one hook. Lua-initiated mutations are **not undoable** in this phase: they bypass
`GuiService::handleMessage`
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
  `guiservice.cpp:173-174` become `session->setActiveGraph (index)`; otherwise
  Lua-initiated activation and hooks diverge from the UI path.

### Hook dispatch points

Only `SessionProxy` fires topology actions: `graph.added`, `graph.removing`,
`graph.removed`, `graph.moved`, `graph.activated`, `node.added`, `node.removing`,
`node.removed`, `connection.added`, `connection.removed`. Pre-hooks (`*.removing`) exist
so the GUI can close plugin windows *before* the processor dies regardless of who
initiated the removal.

- `reload()` runs under `HookBus::ScopedSuspend`, so session load fires nothing per node
  (`GraphManager::setNodeModel` and `IONodeEnforcer` operate below the proxy anyway) and
  the internal `setActiveGraph` during load does not leak `graph.activated`.
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

## Tests

`test/SessionProxyTests.cpp`, suite `SessionProxyTests`, using `element::test::context()`
([TestMain.cpp:15-23](../../test/TestMain.cpp#L15), which activates services so the proxy
is installed) and a test node provider extracted from `CountingNodeProvider`
([SessionLoadBenchTests.cpp:58-82](../../test/SessionLoadBenchTests.cpp#L58)) into
`test/fixture/TestNodeProvider.h`:

- `ProxyInstalledAndCleared` — own `Context`, `services().activate()/deactivate()`.
- `AddGraphAttachesEngine` — `session->addGraph` → `getObject()` non-null, `graph.added` once.
- `AddNodeCreatesProcessor`, `ConnectDisconnect` (arc present in `arcs`, hooks fired).
- `RemoveNodeOrdering` — `node.removing` before `node.removed`.
- `RemoveGraphFixesActive`, `SetActiveGraphSyncsEngine` (`engine->getActiveGraphIndex()`).
- `ReloadFiresNoPerNodeHooks` — `loadData` + `sessionReloaded()` → zero `node.added`.
- `FallbackWithoutProxy` — bare `Context`, `Graph::addNode` adds a tree child, `getObject()` null.

Register with `add_test (NAME "SessionProxyTests" COMMAND test_element --run_test=SessionProxyTests)`.
Existing `SessionTests` keep covering the pure-tree path unchanged. Remember: a
`GraphNode` driven by `GraphManager` must be heap-allocated (`ProcessorPtr keep (new GraphNode (ctx))`).

## Risks / not verified

- Headless attach with real graphs: `SessionTests::MoveEngineGraph` proves
  `AudioEngine::addGraph` works without a device and `test::context()` runs
  `sessionReloaded()` with zero graphs, but the full `attach` + `setRootNode`
  (`setPlayConfigFor (devices)`) path with real graphs has no current test; sample
  rate/block size may be 0 headless.
- `GraphManager::addNode` shows *modal* alerts on instantiation failure; the proxy's
  pre-validation must cover every path or a headless test hangs. What `createGraphNode`
  does with unknown identifiers was not traced.
- Engine-initiated active-graph changes (MIDI program change,
  [audioengine.cpp:657-665](../../src/engine/audioengine.cpp#L657), writes `tags::active`
  from an async update) bypass the proxy. If `graph.activated` must cover them, the
  proxy needs a `ValueTree::Listener` on the `graphs` child with a self-change flag.
  Follow-up.
- `PluginProcessor::reloadEngine` → `reload()` must keep the full detach/re-attach
  behaviour; `prepareExternalPlayback` re-preparation was not traced.
- Re-entrant hooks (a handler adding a node from `node.added`) work through the depth
  guard but interleave `GraphManager::changed()` broadcasts. Untested territory.
- `RootGraphHolder` touches `Processor` internals as `friend class EngineService`
  ([processor.hpp:539](../../include/element/processor.hpp#L539)); `SessionProxy` may
  need adding to the friend list.
- `new_nodetype<Graph>`'s index metamethod already returns non-graph children as
  `Graph` today; the `Graph (const Node&)` constructor asserts type == Graph in debug
  builds. The `sol::make_object` split in session-scripts.md is required, not optional.
- The #1184 issue text was not read; the split-primitive intent is inferred from the
  `Session::moveGraph` doc comment and `EngineService::moveGraph`.
