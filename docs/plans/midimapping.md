# MIDI Mapping System Refactor

## Goal

Replace the three-layer `Controller` → `Control` → `ControllerMap` system with a single
flat `MidiMapping` model stored on the **Session**. Remove the dedicated "Controllers"
authoring workflow: a user just hits **Learn**, wiggles a parameter, wiggles a MIDI
control, and the mapping exists. Targets expand beyond plugin parameters to include
session-level things (tempo, transport).

The **Learn/capture** capability is the single most important behavior to preserve.

This document is written to be implemented in small, independently testable phases. Each
phase lists its unit tests and an exit criterion. Do not start a phase until the previous
phase's tests pass.

## Non-Goals

- No CC value scaling/curves (linear 0–127 → 0.0–1.0 only).
- No macro controls (one MIDI control → many targets).
- No MIDI feedback / output.
- No preservation of the old XML controller import/export format.

## Current System (for reference)

| Concern | Current location |
|---|---|
| Data model | `include/element/controller.hpp`, `src/controller.cpp` — `Control`, `Controller`, `ControllerMap` (all `Model` subclasses) |
| Runtime routing | `src/engine/mappingengine.hpp/.cpp` — `MappingEngine`, `ControllerMapInput`, `MidiNoteControllerMap`, `MidiCCControllerMapHandler` |
| Learn orchestration | `src/services/mappingservice.{hpp,cpp}` — `MappingService`, `AudioProcessorParameterCapture` |
| Device lifecycle | `src/services/deviceservice.{hpp,cpp}` — `DeviceService` |
| Session storage | `include/element/session.hpp` (`getNumControllers`, `getController`, `getNumControllerMaps`, `getControllerMap`, `findControllerById`, `cleanOrphanControllerMaps`, `ControllerMapObjects`), `src/session.cpp` |
| Tags / types | `include/element/tags.hpp` — `EL_TAG(Control/Controller/ControllerMap)`, props `controllers`, `maps`, `control`, `controller`, `inputDevice`, `midiChannel`, `parameter`, `mappingData`, `tempo` |
| UI | `src/ui/controllersview.{hpp,cpp}`, `src/ui/controllermapsview.{hpp,cpp}` |

### Two facts that drive this redesign

1. **Capture is coupled to pre-defined controls.** In
   `ControllerMapInput::handleIncomingMidiMessage` ([mappingengine.cpp:405](src/engine/mappingengine.cpp#L405))
   incoming MIDI is filtered against `controllerNumbers`/`noteNumbers` bitmasks that are
   built from controls *already defined* on a `Controller`, and `captureNextEvent` is
   handed a pre-existing `Control`. So today you can only "learn" something you already
   authored. The new capture path **must accept arbitrary incoming MIDI** and synthesize
   the mapping from the raw `MidiMessage` + device id. This is the core of "cut out the
   middle man."

2. **Routing is welded to JUCE MIDI hardware.** `ControllerMapInput` is a
   `MidiInputCallback` registered with `MidiEngine`, and the handlers hold live JUCE
   `Parameter` pointers. None of it can be unit tested without real devices. The refactor
   splits a **pure, testable core** (message → target action) from a **thin hardware
   adapter**.

## Target Architecture

```
Session ValueTree
└── <midiMappings>                      (tags::midiMappings, replaces controllers + maps)
    └── <MidiMapping version=1>         (types::MidiMapping)
         uuid           : String
         name           : String        (user label, optional)
         device         : String        (JUCE MIDI input identifier; "" = any device)
         eventType      : String        ("note" | "controller")
         eventId        : int           (0–127: note number or CC number)
         midiChannel    : int           (0 = omni, 1–16)
         toggle         : bool          (notes only: latch vs momentary; default false)
         targetType     : String        ("parameter" | "tempo" | "transport")
         node           : String        (uuid; targetType == "parameter")
         parameter      : int           (index, or -2/-3/-4 special; targetType == "parameter")
```

Runtime split into three pieces:

```
        hardware                     pure / unit-testable
   ┌──────────────────┐        ┌──────────────────────────────────┐
   │ MidiInputRouter  │        │ MappingEngine                     │
   │ (MidiInputCallback)─process(deviceId, msg)──►  - bindings: Vec<Binding>
   │  one per device  │        │  - capture state                  │
   └──────────────────┘        │  process(deviceId, msg):          │
                               │     if capturing → captureEvent   │
                               │     else for each binding.matches │
                               │         binding.target->apply(msg)│
                               └───────────────┬───────────────────┘
                                               │ owns
                                               ▼
                                  MappingTarget (interface)
                                   ├── ParameterTarget   (node + index)
                                   ├── TempoTarget
                                   └── TransportTarget
```

### New / changed classes

1. **`MidiMapping : Model`** — new `include/element/midimapping.hpp` (+ `src/midimapping.cpp`
   if needed). Pure data. Follows existing `Model` idioms (`EL_MODEL_GETTER/SETTER`,
   `stabilize*` in a `setMissingProperties()`, `EL_MIDIMAPPING_VERSION 1`).
   - `bool isNoteEvent() / isControllerEvent()`
   - `bool matches(const MidiMessage&) const` — eventType + eventId + channel (0 = omni)
   - `MidiMessage getMidiMessage() const`
   - typed getters: `getDevice`, `getEventType`, `getEventId`, `getMidiChannel`,
     `isToggle`, `getTargetType`, `getNodeUuid`, `getParameterIndex`
   - static factory `MidiMapping::fromCapture(deviceId, msg, targetType, node, param)`
     that builds a fully-formed ValueTree — used by both Learn and tests.

2. **`MappingTarget`** (interface) — new `src/engine/mappingtarget.hpp`.
   ```cpp
   class MappingTarget {
   public:
       virtual ~MappingTarget() = default;
       virtual bool isValid() const = 0;
       virtual void apply (const juce::MidiMessage&, bool toggle) = 0;
   };
   ```
   Implementations:
   - `ParameterTarget` — wraps the begin/setValueNotifyingHost/end gesture logic and the
     special Enabled/Bypass/Mute async handling currently spread across the two handler
     classes (`MidiNoteControllerMap`, `MidiCCControllerMapHandler`). CC → absolute value;
     note → toggle or momentary.
   - `TempoTarget`, `TransportTarget` — session/engine-level (see Open Questions for the
     exact set-tempo API).
   - A factory `std::unique_ptr<MappingTarget> createTarget(const MidiMapping&, Session&, ...)`
     keeps construction in one switchable place and is the seam for adding target types.

3. **`MappingEngine`** (rewritten, same file) — owns `Vec<Binding>` where
   `Binding { MidiMapping mapping; std::unique_ptr<MappingTarget> target; }`. Public API:
   - `void rebuild(SessionPtr)` — clear + create a binding per `MidiMapping`.
   - `void process(const juce::String& deviceId, const juce::MidiMessage&)` — **pure**, the
     unit-test entry point. Routes or captures.
   - `void capture(bool)`, `capturedSignal()`, `getCapturedDevice()`,
     `getCapturedMessage()` — capture now stores the **raw device id + message**, not a
     `Control`.
   - Hardware registration (per device) moves to a small internal
     `MidiInputRouter : juce::MidiInputCallback` that just forwards to `process()`. Not unit
     tested.

4. **`MappingService`** (simplified) — keep `AudioProcessorParameterCapture` essentially
   as-is (it already cleanly captures "which parameter did the user touch" across the
   whole session, including Enabled/Bypass/Mute). Rewrite `onControlCaptured` to build a
   `MidiMapping` from `engine.getCapturedDevice()/getCapturedMessage()` and add it to
   `session.getMidiMappingsValueTree()`, then `engine.rebuild(session)` so it is live
   immediately. Add a `targetType`/pre-targeted-node field to the capture state so the
   Node view can pre-aim Learn at the selected node.

5. **`DeviceService`** — drop controller add/remove and the XML import. `refresh()` becomes
   a thin `context().mapping().rebuild(session)`. Keep "list available MIDI input devices"
   for the UI device dropdown.

6. **`Session`** — add `getMidiMappingsValueTree()`, `getNumMidiMappings()`,
   `getMidiMapping(int)`, `findMidiMappingById(Uuid)`, `addMidiMapping(MidiMapping)`,
   `removeMidiMapping(const MidiMapping&)`, and a `cleanOrphanMidiMappings()` (drop
   mappings whose target node no longer exists — mirrors `cleanOrphanControllerMaps`).
   Ensure the root session ValueTree creates the `<midiMappings>` child.

### Classes to delete (Phase 6, last)

`Control`, `Controller`, `ControllerMap`, `ControllerMapObjects`, `ControllerMapInput`,
`MidiNoteControllerMap`, `MidiCCControllerMapHandler`, `ControllersView`,
`ControllerMapsView`, and the session `controllers`/`maps` accessors. Done last so earlier
phases can read legacy data for migration.

## Behavior Simplifications (decisions — flag if any are wrong)

The current system supports: momentary, toggle, `toggleValue` threshold,
`toggleEqualsOrHigher` vs `toggleEquals` modes, and `inverseToggle`. Proposed reduction:

- **CC → parameter:** linear `value/127.0`. (No threshold, no inverse.)
- **Note → parameter:** `momentary` by default (on = 1.0, off = 0.0). Optional `toggle`
  bool latches on each note-on. (No inverse, no threshold.)
- **Special params (Enabled/Bypass/Mute):** for CC, treat `>= 64` as on; for notes, same
  momentary/toggle rule. Preserve the message-thread async update used today.

If users actually rely on threshold/inverse modes, that's the one place to push back
before deleting `Control`.

## Learn / Capture Flow (must be preserved)

Two phases, same UX as today, but capture now works for **any** MIDI control.

```
Phase 1 — capture parameter
  Learn clicked → MappingService::learn(true)
    capture.addNodes(session)         // listens to every node param + enable/bypass/mute
    state = CaptureParameter
  user moves a plugin control
    Mappable::controlValueChanged → capture fires callback(node, paramIndex)
    onParameterCaptured: store {node, param}; engine.capture(true); state = CaptureControl

Phase 2 — capture MIDI  (THE KEY CHANGE)
  user moves a knob / plays a note on ANY connected device
    MidiInputRouter::handleIncomingMidiMessage → engine.process(deviceId, msg)
    engine, while capturing, stores raw {deviceId, msg} and fires capturedSignal()
    onControlCaptured:
      build MidiMapping::fromCapture(deviceId, msg, targetType, node, param)
      session.addMidiMapping(mapping)
      engine.rebuild(session)         // mapping is live instantly
      state = CaptureStopped; gui->stabilizeViews()
```

Capture state machine lives in `MappingService` (`CaptureStopped / CaptureParameter /
CaptureControl`, as today). The Node-view Learn pre-sets `{targetType=parameter, node}` so
Phase 1 can be skipped or constrained to that node.

## Migration

On session load, convert legacy `controllers`+`maps` into `midiMappings`. Implement as a
free function so it is directly unit-testable on a hand-built ValueTree:

```cpp
// src/session.cpp (or a migration helper)
void migrateControllerMaps (juce::ValueTree session);
//  for each <ControllerMap> in <maps>:
//    resolve Controller (by uuid) → its <Control> (by uuid)
//    new <MidiMapping>:
//      device      = controller.inputDevice
//      eventType   = control.eventType   (note|controller)
//      eventId     = control.eventId
//      midiChannel = control.midiChannel
//      toggle      = !control.momentary
//      targetType  = "parameter"
//      node        = map.node
//      parameter   = map.parameter
//  append to <midiMappings>; leave legacy trees in place (optional: remove after).
```

Threshold/inverse properties are intentionally dropped during migration (see
Simplifications). Run migration in `Session::setData`/load before `cleanOrphanMidiMappings`.

## UI

1. **`MidiMappingsView`** (new, replaces both old views) — one flat table:
   `Device | Type | ID | Ch | Name | Target`, plus **Learn**, **Add**, **Delete**.
2. **Node view concertina panel** — same table filtered to
   `mapping.getNodeUuid() == currentNode.getUuid()`, with a **Learn** that pre-targets the
   current node. Hook into the existing NavigationConcertina used by the node editor.

UI is intentionally last and is **not** unit tested (manual checklist below).

## Phased Implementation & Tests

Build/run tests (from `docs/building.md`):
```
cmake --build build
ctest --test-dir build --output-on-failure -R MidiMapping
```
Tests use **Boost.Test** (`BOOST_AUTO_TEST_SUITE`), live in `test/`, are auto-globbed by
`test/CMakeLists.txt`, and must be registered with an `add_test(NAME ... --run_test=...)`
line. Fixtures: `test/fixture/TestNode.h` (ports), `Context` + `GraphNode` pattern from
`test/MidiProgramMapTests.cpp`. **A `TestNode` with real parameters is needed** — extend
`TestNode` (or add `test/fixture/ParamTestNode.h`) exposing N `Parameter`s so targets can
be asserted.

---

### Phase 0 — Test fixture
Add a `Processor` fixture exposing real parameters (regular + report enable/bypass/mute).
*Exit:* fixture compiles and a node reports `getParameters().size() > 0`.

### Phase 1 — `MidiMapping` model + tags
Add `tags::midiMappings`, `types::MidiMapping`, new property tags; create `MidiMapping`.
**Tests — `MidiMappingModelTests`:**
- default construction stabilizes all properties.
- `fromCapture` with a CC message → correct eventType/eventId/channel.
- `fromCapture` with a note message → correct eventType/eventId.
- `matches()` : exact CC match; channel omni (0) matches any; specific channel rejects
  others; wrong eventId rejects; note vs controller discrimination.
- ValueTree round-trip: build → `data().toXml()` → reparse → equal.
*Exit:* suite green.

### Phase 2 — `MappingTarget` + `ParameterTarget`
Implement the target interface, `ParameterTarget`, and `createTarget` factory.
**Tests — `MappingTargetTests`** (no MIDI hardware, no engine):
- CC 127 → parameter == 1.0; CC 0 → 0.0; CC 64 → ~0.5.
- note momentary: note-on → 1.0, note-off → 0.0.
- note toggle: two note-ons flip 0→1→0.
- special params: CC ≥64 enables / <64 disables (assert on node state).
- invalid node / out-of-range index → `isValid() == false`, `apply` is a no-op.
*Exit:* suite green.

### Phase 3 — `MappingEngine` routing + capture
Rewrite engine around `process(deviceId, msg)` and the binding list; add capture storing
raw device+message.
**Tests — `MappingEngineTests`** (drive `process()` directly):
- `rebuild` from a session with one parameter mapping; `process` matching CC updates the
  param; non-matching CC does not.
- device filter: mapping bound to device "A"; `process("B", msg)` is ignored; `""` matches
  any device.
- capture mode: `capture(true)`, `process(dev, msg)` → `capturedSignal` fires once,
  `getCapturedDevice/Message` correct, and the message is **not** routed to targets.
- capture accepts a CC/note that matches **no existing mapping** (the core fix).
*Exit:* suite green.

### Phase 4 — Session storage + migration
Add session accessors, `<midiMappings>` creation, `migrateControllerMaps`,
`cleanOrphanMidiMappings`.
**Tests — `MidiMappingSessionTests`:**
- add/remove/find/count round-trip on a Session.
- save → reload Session ValueTree preserves mappings.
- `migrateControllerMaps`: hand-build a legacy controllers+maps tree (CC control + map to
  node/param) → assert one equivalent `MidiMapping` (device, eventId, channel, node,
  parameter, toggle from `!momentary`).
- `cleanOrphanMidiMappings` drops a mapping whose node uuid is absent.
*Exit:* suite green.

### Phase 5 — Service wiring (Learn) + hardware adapter
Rewrite `MappingService::onControlCaptured` to synthesize a `MidiMapping` and rebuild;
simplify `DeviceService::refresh` to `engine.rebuild`; add `MidiInputRouter` forwarding
hardware MIDI to `engine.process`.
**Test — `MappingLearnTests`** (integration-style, no real device; simulate by calling the
service/engine seams directly):
- simulate Phase 1 (`onParameterCaptured(node, param)`) then Phase 2
  (`engine.process(dev, cc)` with capture armed) → exactly one `MidiMapping` added to the
  session, target node/param correct, and a subsequent `process(dev, cc)` drives the
  parameter (proves "live immediately").
*Exit:* suite green; full `ctest` green.

### Phase 6 — UI + deletion of legacy code
Build `MidiMappingsView` and the Node concertina panel; remove `Control`, `Controller`,
`ControllerMap`, old handlers, old views, and dead session accessors. Update all
references and `test/CMakeLists.txt` `add_test` entries.
*Exit:* project builds with no references to deleted types; full `ctest` green; manual
checklist below passes.

## Manual QA Checklist (Phase 6)
- [ ] Learn from the global view: wiggle param, wiggle knob → mapping appears and works.
- [ ] Learn from the Node concertina pre-targets the selected node.
- [ ] CC drives a continuous parameter; note toggles enable/bypass/mute.
- [ ] Delete removes the mapping and stops routing.
- [ ] Load an **old** session → mappings migrate and still work.
- [ ] Save/reload preserves mappings.
- [ ] Two devices: a mapping bound to device A ignores device B.

## Open Questions
1. **Tempo/transport apply API:** what's the thread-safe call to set session tempo and
   start/stop transport from the message thread? (Confirm before Phase 2's
   `TempoTarget`/`TransportTarget`; ship `ParameterTarget` first regardless.)
2. **Keep threshold/inverse toggle modes?** Plan assumes no. Confirm before deleting
   `Control` in Phase 6.
3. **Delete legacy trees after migration**, or leave them for a release as a safety net?
