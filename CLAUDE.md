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

## Audio Graph Architecture

- **IONodes** (audio/MIDI input/output) require a parent `GraphNode` to be set before ports can be properly initialized.
- `IONode::refreshPorts()` queries the parent graph's port count via `graph->getNumPorts()`. If the parent is null or has zero ports, the IONode will have zero ports.
- When adding IONodes, ensure the parent graph has a valid port count first using `graph->setNumPorts()`.
- Default port counts: 2 channels for audio (stereo), 1 channel for MIDI.
- Message flow for adding nodes: `AddPluginMessage` → `AddPluginAction::perform()` → `EngineService::addPlugin()` → `GraphManager::addNode()`.
