// Copyright 2026. Kushview, LLC
// Author: Buzz Burrowes

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

// Matches e.g.  type = 'DSP'   or   type="DSPUI"   (either quote style, flexible whitespace).
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

/** Scans raw Lua source text for the `type = '...'` declaration in the
    script's trailing return block (the text following a stand-alone
    "return {" line) -- not anywhere else in the file. Returns the type
    string (uppercased, e.g. "DSP" or "DSPUI") via outType, and any
    `dspName = '...'` override via outDspNameOverride, if present.

    Returns false if no return block was found, or the return block has no
    `type` field at all -- either way, the resource is not a node script
    this registry cares about.

    NOTE: this is a lightweight text scan, not a real Lua parse -- it does
    not execute the script. Given this codebase's convention of a single
    return block at the very end of each script, anchoring on "return {"
    is sufficient in practice. If that ever stops holding true, the robust
    fix is to actually execute the chunk through sol2/lua_State and inspect
    the returned table directly, rather than scanning text.
*/
bool extractScriptType (const std::string& source, std::string& outType, std::string& outDspNameOverride)
{
    std::string region = extractReturnBlockRegion (source);
    if (region.empty())
        return false; // no "return {" line found at all -- not a node script

    std::smatch typeMatch;
    if (! std::regex_search (region, typeMatch, kTypeRegex))
        return false;

    outType = toUpperCopy (typeMatch[1].str());

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
    // scripts below.
    std::unordered_map<std::string, size_t> indexByName;
    indexByName.reserve (raw.size());
    for (size_t i = 0; i < raw.size(); ++i)
        indexByName.emplace (raw[i].name, i);

    // 2. Determine, up front and independent of iteration order, each raw
    //    resource's declared `type` (if any) and dspName override. This
    //    must be computed for EVERY resource before any UI-companion
    //    pairing decision below, since pairing needs to know whether the
    //    *candidate companion* positively declares itself as `DSPUI` --
    //    not merely that it exists, and not merely that it isn't `DSP`.
    std::vector<bool> isDspValid (raw.size(), false);
    std::vector<bool> isUiValid (raw.size(), false);
    std::vector<std::string> dspNameOverride (raw.size());

    for (size_t i = 0; i < raw.size(); ++i)
    {
        std::string source (raw[i].data, static_cast<size_t> (raw[i].size));
        std::string type;
        if (! extractScriptType (source, type, dspNameOverride[i]))
            continue; // no return block / no type field -- not a node script

        isDspValid[i] = (type == "DSP");
        isUiValid[i]  = (type == "DSPUI");
    }

    // 3. Decide UI companions. A resource named "<name>ui" is treated as
    //    the UI companion of "<name>" only if:
    //      a) "<name>" is itself a valid DSP script (type == 'DSP'), AND
    //      b) "<name>ui" is itself a valid UI script (type == 'DSPUI').
    //
    //    Requiring an explicit `type = 'DSPUI'` on the companion (rather
    //    than just "isn't DSP") means a same-named resource that happens to
    //    exist for some unrelated reason, or is malformed, or declares some
    //    other type entirely, is never mistaken for a real UI companion.
    //    It also means a genuine standalone DSP script that happens to be
    //    named e.g. "flexui.lua" is never silently swallowed as someone
    //    else's UI companion -- it surfaces as its own top-level entry, and
    //    (per this same rule applied to it) its own potential companion is
    //    looked up as "flexuiui.lua".
    std::vector<bool> isUiCompanion (raw.size(), false);

    for (size_t i = 0; i < raw.size(); ++i)
    {
        if (! isDspValid[i])
            continue;

        auto it = indexByName.find (raw[i].name + "ui");
        if (it == indexByName.end())
            continue;

        size_t j = it->second;
        if (isUiValid[j])
            isUiCompanion[j] = true;
    }

    // 4. Build the final entry list: every resource that is a valid DSP
    //    script and is not itself consumed as another entry's UI companion
    //    becomes a top-level BuiltInScripts entry, with its UI companion
    //    (if any, per the rules above) attached and its display name
    //    resolved (dspName override, or filename-derived name as fallback).
    names.reserve (raw.size()); // upper bound; guarantees c_str() stability below
    scripts.reserve (raw.size());

    for (size_t i = 0; i < raw.size(); ++i)
    {
        if (! isDspValid[i] || isUiCompanion[i])
            continue;

        names.push_back (dspNameOverride[i].empty() ? raw[i].name : dspNameOverride[i]);

        const char* uiScript = nullptr;
        int uiSize = 0;

        auto it = indexByName.find (raw[i].name + "ui");
        if (it != indexByName.end() && isUiValid[it->second])
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