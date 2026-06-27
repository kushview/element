# Copilot Instructions for Element

## General Conventions

- **Always check documentation**: Before making assumptions about APIs, libraries, or tools, consult the official documentation first.
- **Do not make assumptions and guesses**: When uncertain about implementation details, research or ask rather than guessing.
- **Avoid workarounds**: Do things "the right way" by using the proper APIs and intended patterns. Take time to research the correct solution rather than applying quick hacks that create technical debt.
- **KISS (Keep It Simple, Stupid)**: Favor simple, straightforward solutions over complex ones.
- **DRY (Don't Repeat Yourself)**: Avoid code duplication. Extract common functionality into reusable functions or components.

## Code Quality

- Write clear, readable code with descriptive names for variables, functions, and classes.
- Maintain consistency with the existing codebase style and patterns.
- Consider maintainability and future developers who will read the code.

## Headers & Includes

- **Never `using namespace ...` in a header** (repository-wide rule). It leaks the namespace into every translation unit that includes the header. Fully qualify names (e.g. `juce::Component`) in headers instead. A file-local `using namespace juce;` inside a `.cpp` is fine.
- **Do not include `ElementApp.h` in new source files.** It is a legacy helper that declares `using namespace juce;` at public scope, which the codebase is moving away from. Instead include the specific clean umbrella headers under `element/juce/` (e.g. `<element/juce/gui_basics.hpp>`, `<element/juce/audio_basics.hpp>`) or `<element/juce.hpp>` (which does not pull in the juce namespace).

## Documentation

- Use Doxygen-style comments with `/** ... */` for documenting classes, functions, and methods.
- Include a brief description, parameter documentation with `@param`, and return value documentation with `@return`.
- Document what the function does, not how it does it (implementation details belong in inline comments).
- Example:
  ```cpp
  /** Checks if the application can safely shut down.
      
      Determines whether there are any unsaved changes in the current session
      that would be lost on shutdown.
      
      @return true if there are no pending session changes and shutdown can proceed,
              false if the session has unsaved changes
  */
  static bool canShutdown();
  ```

## Data Models (ValueTree)

- Domain objects (e.g. `Session`, `Node`, `Control`, `Controller`) subclass `Model` and wrap a `juce::ValueTree` (`objectData`). They are lightweight value types — copy them freely; identity lives in the underlying tree.
- Are the data representation of a real audio plugin `Processor`.
- Declare new types and property names in `include/element/tags.hpp`: `EL_TAG(MyType)` for a tree type (under `types::`), and a `juce::Identifier` for each property (under `tags::`). Reuse existing tags rather than introducing string duplicates.
- Use the `EL_MODEL_GETTER`/`EL_MODEL_SETTER` macros for property accessors.
- Give each model a version constant (`#define EL_MYTYPE_VERSION 1`) and a private `setMissingProperties()` that calls `stabilizePropertyString`/`stabilizePropertyPOD` to fill defaults. This same method is the place to do in-place migration of legacy properties (see `Control::setMissingProperties` converting legacy `mappingData`).
- Persistence is just the ValueTree serialized to XML; there is no separate DTO layer.

## Services & Context

- App logic lives in `Service` subclasses (`src/services/`) with `activate()`/`deactivate()` lifecycle hooks. Reach another service from within one via `sibling<OtherService>()`.
- Shared singletons are reached through `context()` — e.g. `context().session()`, `context().mapping()`, `context().midi()`.
- Marshal state changes off the audio/MIDI thread to the message thread with `juce::AsyncUpdater`. Plugin parameter changes must be wrapped in `beginChangeGesture()` / `setValueNotifyingHost()` / `endChangeGesture()`.

## Testing

- Tests use **Boost.Test** and live in `test/` (built into the `test_element` console app). `test/CMakeLists.txt` globs all `*.cpp`, but each suite must ALSO be registered with an explicit `add_test(NAME "MySuite" COMMAND test_element --run_test=MySuite)` line — forgetting this is the usual reason a new test "doesn't run."
- Write suites with `BOOST_AUTO_TEST_SUITE(Name)` / `BOOST_AUTO_TEST_CASE(...)` / `BOOST_AUTO_TEST_SUITE_END()`.
- Standard fixtures: construct a `Context`, then `GraphNode graph(context)` and `graph.addNode(new SomeNode(...))` (see `test/MidiProgramMapTests.cpp`). Reusable nodes live in `test/fixture/` (e.g. `TestNode.h`).
- Build and run:
  ```
  cmake --build build
  ctest --test-dir build --output-on-failure        # or -R MySuite for one suite
  ```
- Prefer designing engine/runtime code so its core logic is callable without real hardware (e.g. a plain `process(...)` method), so it can be unit tested directly.

## Audio Graph Architecture

- **IONodes** (audio/MIDI input/output) require a parent `GraphNode` to be set before ports can be properly initialized.
- `IONode::refreshPorts()` queries the parent graph's port count via `graph->getNumPorts()`. If the parent is null or has zero ports, the IONode will have zero ports.
- When adding IONodes, ensure the parent graph has a valid port count first using `graph->setNumPorts()`.
- Default port counts: 2 channels for audio (stereo), 1 channel for MIDI.
- Message flow for adding nodes: `AddPluginMessage` → `AddPluginAction::perform()` → `EngineService::addPlugin()` → `GraphManager::addNode()`.
