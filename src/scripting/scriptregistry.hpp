// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

namespace element {

/** A built-in Lua script pair: a DSP script and its optional companion UI script.

    Naming convention: DSP resource is "<name>.lua", UI resource (if present)
    is "<name>ui.lua" -- e.g. "amp.lua" pairs with "ampui.lua".

    A script is only exposed here if its source contains a top-level return
    block declaring `type = 'DSP'`, e.g.:

        return {
            type        = 'DSP',
            layout      = amp_layout,
            process     = amp_process
        }

    That return block may also optionally declare `dspName = '...'`, which
    overrides the display name (otherwise the DSP resource's filename, minus
    the .lua extension, is used).
*/
struct BuiltInScripts
{
    const char* name;
    const char* dspScript;
    const int   dspSize;
    const char* uiScript;   // nullptr if this script has no companion UI
    const int   uiSize;     // 0 if uiScript is nullptr
};

/** Singleton registry of all built-in Lua scripts embedded into the binary via
    juce_add_binary_data() (see scripts/CMakeLists.txt, NAMESPACE `scripts`)
    that declare themselves as DSP script nodes.

    The script list is discovered entirely at runtime:
      1. Every embedded resource is enumerated from scripts::namedResourceList.
      2. Resources are paired up using the "<name>" / "<name>ui" naming
         convention (the ui one becomes a companion, not its own entry).
      3. Each remaining candidate's source is scanned for a `type = 'DSP'`
         return block; candidates without one, or whose `type` is something
         else (e.g. view.lua's `type = 'View'`), are dropped entirely.
      4. If that return block also declares `dspName = '...'`, it is used as
         the entry's display name; otherwise the filename-derived name is used.

    Nothing is hardcoded, so new scripts dropped into scripts/ are picked up
    automatically without touching this class.

    Populated lazily on first access. Thread-safe by virtue of C++11
    function-local static initialization guarantees.
*/
class ScriptRegistry
{
public:
    /** Returns the single shared instance, constructing it on first call. */
    static ScriptRegistry& instance();

    /** All discovered built-in DSP scripts. */
    const std::vector<BuiltInScripts>& getScripts() const noexcept { return scripts; }

    /** Looks up a script by its display name (e.g. "amp", or its dspName
        override if one was declared). Returns nullptr if not found.
    */
    const BuiltInScripts* findByName (const char* name) const noexcept;

    // Non-copyable, non-movable: there is exactly one registry.
    ScriptRegistry (const ScriptRegistry&) = delete;
    ScriptRegistry& operator= (const ScriptRegistry&) = delete;

private:
    ScriptRegistry();
    ~ScriptRegistry() = default;

    std::vector<BuiltInScripts> scripts;

    // Owns the display-name strings that BuiltInScripts::name points into.
    // Capacity is reserved up front in the constructor so push_back never
    // reallocates and invalidates c_str().
    std::vector<std::string> names;
};

} // namespace element