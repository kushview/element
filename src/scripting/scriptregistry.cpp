// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cctype>
#include <cstring>
#include <regex>
#include <unordered_map>
#include "scriptregistry.hpp"
#include "luascripts.hpp"

namespace element {

namespace {

// One raw embedded resource, resolved to its extension-stripped base name.
// `data`/`size` point directly at the static BinaryData buffer -- NOT
// necessarily null-terminated, so always paired with `size`.
struct RawResource
{
    std::string name;   // e.g. "amp", "ampui", "channelize"...
    const char* data;
    int         size;
};

std::string stripLuaExtension (const std::string& filename)
{
    static const std::string ext = ".lua";
    if (filename.size() > ext.size()
        && filename.compare (filename.size() - ext.size(), ext.size(), ext) == 0)
        return filename.substr (0, filename.size() - ext.size());
    return filename;
}

bool endsWith (const std::string& s, const std::string& suffix)
{
    return s.size() >= suffix.size()
        && s.compare (s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string toUpperCopy (std::string s)
{
    for (auto& c : s)
        c = static_cast<char> (std::toupper (static_cast<unsigned char> (c)));
    return s;
}

// Matches a the LAST line in the script containing ONLY "return {" (ignores 
// whitespace). This is what anchors the start of the script's return block
// so the field scan below can't accidentally match an unrelated 
// `type`/`dspName` local variable earlier in the file.
//
// NOTE: matched one line at a time via regex_match() rather than using the
// std::regex::multiline flag + ^/$ over the whole file -- MSVC's STL has
// never implemented std::regex::multiline (a long-standing gap versus
// libstdc++/libc++), so ^/$ are used here in their default, per-call meaning
// of "start/end of the string being matched", with that string being a
// single line.
const std::regex kReturnBlockStartLineRegex (R"(^[ \t]*return[ \t]*\{[ \t]*\r?$)");

// Matches e.g.  type = 'DSP'   or   type="DSP"   (either quote style, flexible whitespace).
const std::regex kTypeRegex (R"(\btype\s*=\s*['"]([^'"]+)['"])");

// Matches e.g.  dspName = 'Amplifier'
const std::regex kDspNameRegex (R"(\bdspName\s*=\s*['"]([^'"]+)['"])");

/** Returns the substring of `source` starting right after the LAST line
    containing only "return {" (see kReturnBlockStartLineRegex), or an empty
    string if no such line is found.

    Scripts commonly contain earlier "return {" lines too (e.g. inside a
    layout() helper function) -- only the final, module-level return block is
    the one that actually declares this script's type/dspName, so every
    matching line is checked and the last one found wins.
*/
std::string extractReturnBlockRegion (const std::string& source)
{
    size_t pos = 0;
    bool found = false;
    size_t regionStart = 0; // valid only when found == true

    while (pos <= source.size())
    {
        size_t newlinePos = source.find ('\n', pos);
        std::string line = (newlinePos == std::string::npos)
                                ? source.substr (pos)
                                : source.substr (pos, newlinePos - pos);

        if (std::regex_match (line, kReturnBlockStartLineRegex))
        {
            found = true;
            regionStart = (newlinePos == std::string::npos) ? source.size() : newlinePos + 1;
            // keep scanning -- do NOT return here, a later match should win
        }

        if (newlinePos == std::string::npos)
            break;

        pos = newlinePos + 1;
    }

    return found ? source.substr (regionStart) : std::string();
}

/** Scans raw Lua source text for a `type = 'DSP'` declaration, but only
    within the script's trailing return block (the text following a
    stand-alone "return {" line) -- not anywhere else in the file. This is
    what lets e.g. `local type = something` earlier in a script's body avoid
    being mistaken for the return block's `type` field.

    NOTE: this is a lightweight text scan, not a real Lua parse -- it does
    not execute the script. Given this codebase's convention of a single
    return block at the very end of each script, anchoring on "return {"
    is sufficient in practice. If that ever stops holding true, the robust
    fix is to actually execute the chunk through sol2/lua_State and inspect
    the returned table directly, rather than scanning text.
*/
bool isDspScript (const std::string& source, std::string& outDspNameOverride)
{
    std::string region = extractReturnBlockRegion (source);
    if (region.empty())
        return false; // no "return {" line found at all -- not a node script

    std::smatch typeMatch;
    if (! std::regex_search (region, typeMatch, kTypeRegex))
        return false;

    if (toUpperCopy (typeMatch[1].str()) != "DSP")
        return false;

    std::smatch nameMatch;
    if (std::regex_search (region, nameMatch, kDspNameRegex))
        outDspNameOverride = nameMatch[1].str();

    return true;
}

} // namespace

ScriptRegistry& ScriptRegistry::instance()
{
    // Function-local static: constructed thread-safely on first call,
    // avoids static initialization order issues entirely.
    static ScriptRegistry registry;
    return registry;
}

ScriptRegistry::ScriptRegistry()
{
    // 1. Pull every embedded resource out of the generated BinaryData table
    //    and resolve it to a base name (original filename minus ".lua").
    std::vector<RawResource> raw;
    raw.reserve (static_cast<size_t> (scripts::namedResourceListSize));

    for (int i = 0; i < scripts::namedResourceListSize; ++i)
    {
        const char* resourceName = scripts::namedResourceList[i];

        int dataSize = 0;
        const char* data = scripts::getNamedResource (resourceName, dataSize);
        if (data == nullptr)
            continue; // malformed entry, skip rather than crash

        const char* originalFilename = scripts::getNamedResourceOriginalFilename (resourceName);
        std::string baseName = stripLuaExtension (originalFilename != nullptr ? originalFilename
                                                                                : resourceName);

        raw.push_back ({ std::move (baseName), data, dataSize });
    }

    // Index by file-derived name for O(1) lookups while pairing DSP <-> UI
    // scripts and resolving UI companions below. This index intentionally
    // uses filenames, not dspName overrides, since ui-companion filenames
    // (e.g. "ampui") are matched against the DSP script's *file* name.
    std::unordered_map<std::string, size_t> indexByName;
    indexByName.reserve (raw.size());
    for (size_t i = 0; i < raw.size(); ++i)
        indexByName.emplace (raw[i].name, i);

    // 2. Determine which entries are actually "<base>ui" companions of
    //    another real entry, so they don't also show up as standalone nodes.
    std::vector<bool> isUiCompanion (raw.size(), false);

    for (size_t i = 0; i < raw.size(); ++i)
    {
        if (! endsWith (raw[i].name, "ui"))
            continue;

        std::string baseName = raw[i].name.substr (0, raw[i].name.size() - 2);
        if (baseName.empty())
            continue;

        auto it = indexByName.find (baseName);
        if (it != indexByName.end() && it->second != i)
            isUiCompanion[i] = true;
    }

    // 3. Build the final entry list: everything that (a) isn't someone else's
    //    UI companion, and (b) declares `type = 'DSP'` in its return block,
    //    becomes a top-level BuiltInScripts entry, with its UI companion (if
    //    any) attached and its display name resolved (dspName override, or
    //    filename-derived name as the fallback).
    names.reserve (raw.size()); // upper bound; guarantees c_str() stability below
    scripts.reserve (raw.size());

    for (size_t i = 0; i < raw.size(); ++i)
    {
        if (isUiCompanion[i])
            continue;

        std::string source (raw[i].data, static_cast<size_t> (raw[i].size));

        std::string dspNameOverride;
        if (! isDspScript (source, dspNameOverride))
            continue; // not a DSP node script (e.g. a shared support module) -- skip

        names.push_back (dspNameOverride.empty() ? raw[i].name : dspNameOverride);

        const char* uiScript = nullptr;
        int uiSize = 0;

        auto it = indexByName.find (raw[i].name + "ui");
        if (it != indexByName.end())
        {
            uiScript = raw[it->second].data;
            uiSize   = raw[it->second].size;
        }

        // dspSize/uiSize are const members, so the struct must be built in
        // one aggregate-initialization step rather than default-constructed
        // and assigned to afterward.
        scripts.push_back (BuiltInScripts { names.back().c_str(),
                                             raw[i].data,
                                             raw[i].size,
                                             uiScript,
                                             uiSize });
    }
}

const BuiltInScripts* ScriptRegistry::findByName (const char* name) const noexcept
{
    for (auto& s : scripts)
        if (std::strcmp (s.name, name) == 0)
            return &s;

    return nullptr;
}

} // namespace element