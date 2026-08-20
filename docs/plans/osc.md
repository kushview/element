# OSC Control at the Node/Parameter Level

## Context

Element's OSC support today ([oscservice.cpp](src/services/oscservice.cpp)) handles only two fixed addresses (`/element/command` — a no-op stub — and `/element/engine` for samplerate). The goal is full bidirectional OSC control of node parameters: **set** values, **query** current values with OSC replies, and push **feedback** to a configured client so bidirectional control surfaces (TouchOSC etc.) stay in sync.

Decisions made with the user:
- Nodes addressed by **numeric node id or slug** (sanitized name).
- **Full bidirectional** scope (set + query + feedback, with new client settings/UI).
- **No gesture bracketing** for now — plain `setValueNotifyingHost` (defer begin/end gestures).

Hard constraint discovered: `juce::OSCReceiver` callbacks do not expose the sender's address, so replies/feedback all go to a single **configured OSC client host/port** (new settings), not back to the sender.

## Address spec

```
/element[/graph/<graph>]/node/<node>/param/<index>   [value]   – regular parameter
/element[/graph/<graph>]/node/<node>/enabled          [value]
/element[/graph/<graph>]/node/<node>/bypass           [value]
/element[/graph/<graph>]/node/<node>/mute             [value]
/element[/graph/<graph>]/node/<node>/inputgain        [value]
/element[/graph/<graph>]/node/<node>/outputgain       [value]
/element/command <string>                                       – existing, folded in
/element/engine  <string> [args…]                               – existing, folded in
```

- `<graph>`: decimal index (`session->getGraph(i)`) or slug; **omitted → active graph** (node ids are only unique per graph).
- `<node>`: all-digits → uint32 engine node id via `Node::getNodeById` (recursive); otherwise slug.
- **Slug rule**: name (fallback `getDisplayName()`), lowercased, runs of non-`[a-z0-9]` → single `-`, trimmed; collisions resolve to **first match in depth-first order** (deterministic; ids are the exact address).
- **Set**: one arg, float32 or int32; regular params normalized 0..1 clamped; specials on ⇔ value ≥ 0.5; gains normalized over [-60, +6] dB (same math as `ParameterTarget::applyGain`).
- **Query**: same address, zero args → reply with the **incoming address echoed verbatim** + one float32 (so one widget address works both ways).
- Wildcard patterns: optional later phase via `juce::OSCAddressPattern::matches` over enumerated candidates.

## Implementation

### 1. `ParameterTarget` value API (refactor, DRY)
[mappingtarget.hpp](src/engine/mappingtarget.hpp) / [mappingtarget.cpp](src/engine/mappingtarget.cpp): add public value-based methods and rewire the private MIDI apply paths through them:
- `void setNormalizedValue (float)` — `setValueNotifyingHost`, no gestures
- `void setSpecial (bool on)` — extracted from `applySpecial` (enabled → `object->setEnabled` + `tags::enabled`; bypass → `suspendProcessing` + `tags::bypass`; mute → `model.setMuted`)
- `void setGainNormalized (float)` — extracted from `applyGain`
- `float getNormalizedValue() const` — new, for query replies
- `ParameterPtr getParameter() const` — for feedback observation

Existing `MappingTargetTests` must stay green.

### 2. `OSCRouter` — new testable core (no networking)
New files `src/services/oscrouter.hpp/.cpp` (glob picks them up). Message-thread only; constructed with `SessionPtr`.
- `detail::oscSlugify (const juce::String&)` — free function, unit-testable.
- `Result handleMessage (const juce::OSCMessage&)` where `Result { bool handled; std::vector<juce::OSCMessage> replies; }`.
- `registerHandler (address, std::function<void (const OSCMessage&)>)` — exact-address handlers checked first; used to fold in the existing `/element/command` and `/element/engine` listeners (delete `CommandOSCListener`/`EngineOSCListener` structs).
- **Cache**: `std::map<juce::String, ResolvedTarget>` keyed by full address string → `{ Node, std::unique_ptr<ParameterTarget> }`; lazy fill on miss (graph → node by id or slug DFS → `ParameterTarget(node, index)` with special-param enums from [processor.hpp](include/element/processor.hpp) for the named specials); `clearCache()` for wholesale invalidation.
- `Signal<void (const juce::String& address, ParameterPtr)> sigObserveParameter` — fired on successful set/query of regular params; the service uses it to start feedback observation.

### 3. Settings + client sender
[settings.hpp](include/element/settings.hpp) / [settings.cpp](src/settings.cpp), following the exact `oscHost*` pattern (settings.cpp:325-354): `isOscClientEnabled/setOscClientEnabled`, `getOscClientHost/setOscClientHost` (default `"127.0.0.1"`), `getOscClientPort/setOscClientPort` (default `9001`).
`OSCService::refreshWithSettings`: also reconnect `impl->sender` to the client host/port when enabled (alert on failure like the host path).

### 4. Service wiring
[oscservice.cpp](src/services/oscservice.cpp) `Impl`:
- Replace per-address listeners with a catch-all `OSCReceiver::Listener<MessageLoopCallback>` (message-thread dispatch → safe for models and `setValueNotifyingHost`). Handle bundles by recursing elements.
- `oscMessageReceived`: `router->handleMessage(msg)`; send replies via `sender` only when connected.
- Cache invalidation: in `activate()`, connect `sibling<SessionService>()->sigSessionLoaded` and `context().audio()->sigNodeRemoved` ([engine.hpp:130](include/element/engine.hpp#L130)) → `router->clearCache()` + clear feedback observers; store `SignalConnection`s, disconnect in `deactivate()`. (No node-added signal exists; lazy resolution covers additions.)

### 5. Feedback
Private `FeedbackController` struct in the Impl: `std::map<juce::String, std::unique_ptr<ParameterObserver>>` keyed by address, populated from `sigObserveParameter` (only params touched/queried via OSC are observed — bounded cost). Each observer's `sigValueChanged` sends `OSCMessage(address, value)` via `sender`, skipping unchanged values; `ParameterObserver`'s built-in 50 Hz/backoff throttling applies. Cleared on session load / node removal (observers hold `ParameterPtr` refs — must not outlive node removal).
v1 limitation: push feedback for regular parameters only; specials support set + query but no push (they aren't `Parameter`s; a later ValueTree-listener approach must use a persistent member tree, never `addListener` on `model.data()`).

### 6. Preferences UI
[preferences.cpp](src/ui/preferences.cpp) `OSCSettingsPage` (lines 123-204): add three rows mirroring the host rows — client enabled `SettingButton`, editable client-host `TextEditor` (unlike the read-only host field), IncDec port `Slider`; changes write settings + `triggerAsyncUpdate()` → existing `refreshWithSettings(true)` plumbing.

## Tests

New `test/OSCRouterTests.cpp`, suite `OSCRouterTests`; add `add_test(NAME "OSCRouterTests" COMMAND test_element --run_test=OSCRouterTests)` to [test/CMakeLists.txt](test/CMakeLists.txt). juce_osc is linked PUBLIC so `juce::OSCMessage` constructs directly. Fixture per `MidiMappingSessionTests`/`MappingTargetTests`: `Context` + `session()`, `makeNode` helper (extend MappingTargetTests:19-25 pattern to set `tags::id`/`tags::name`) backed by `test/fixture/ParamTestNode.h`.

Cases: slugify rule; set by id and by slug (float + int arg, clamping); slug collision first-match; specials set object + model properties; gain math; query reply echoes address with correct value; graph prefix and active-graph default; unhandled addresses (foreign namespace, missing node, bad index) → `handled == false`, no crash; registered-handler fold-in short-circuits; cache invalidation re-resolves after `clearCache()`; `sigObserveParameter` fires with exact address + correct `ParameterPtr`.

## Commit-sized phases

1. `ParameterTarget` value API refactor (+ keep MappingTargetTests green).
2. `OSCRouter` + full test suite (everything testable before networking).
3. Settings keys + sender connection.
4. Service wiring: catch-all listener, fold-in handlers, replies, invalidation signals.
5. Feedback controller + preferences UI rows.
6. (Optional) wildcard support; docs for the address spec; `util/format.py` pass.

## Verification

- `cmake --build build && ctest --test-dir build --output-on-failure -R OSCRouterTests` (plus `-R MappingTargetTests` after phase 1).
- End-to-end: run Element, enable OSC host (port 9000) and client (9001) in Preferences; from a shell use an OSC tool (e.g. `oscsend`/the existing `test/osc/node-client` JS client) to send `/element/node/<id>/param/0 0.5` and watch the UI knob move; send the zero-arg form and confirm the reply/feedback arrives on the client port (e.g. `oscdump 9001`).
