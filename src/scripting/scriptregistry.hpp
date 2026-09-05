// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later
// Author: Buzz Burrowes

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

    A "<name>ui" resource is only ever treated as <name>'s UI companion if it
    itself declares `type = 'DSPUI'` in its own return block. This means a
    <name>ui resource that happens to exist for some unrelated reason (or is
    itself a standalone DSP script, or declares some other type entirely) is
    never mistakenly swallowed as a companion -- see scriptregistry.cpp for
    the full pairing rules.
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
      2. Each resource's declared `type` (if any) is determined by scanning
         its trailing return block -- 'DSP', 'DSPUI', or anything else
         (which is dropped, e.g. view.lua's `type = 'View'`).
      3. Resources named "<name>ui" are paired to "<name>" as a UI companion
         only when <name> is a valid DSP script AND "<name>ui" is itself a
         valid DSPUI script -- never merely by name existing.
      4. If a DSP script's return block declares `dspName = '...'`, it is
         used as the entry's display name; otherwise the filename-derived
         name is used.

    Nothing is hardcoded, so new scripts dropped into scripts/ are picked up
    automatically without touching this class.

    Populated lazily on first access. Thread-safe by virtue of C++11
    function-local static initialization guarantees (construction only --
    the underlying vectors are populated once during construction and never
    mutated afterward, so concurrent reads via getScripts()/findByName()
    after that first call are safe).
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

        The returned pointer refers to storage owned by the registry and
        remains valid for the lifetime of the application (the registry is
        a function-local static that is never destroyed until program exit).
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