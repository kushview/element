# LuaJIT Migration — Assessment

Sizing the job of moving Element's embedded Lua from vendored **Lua 5.4**
(`src/lua/src`, sol2 bindings) to **LuaJIT**, or supporting both.

## Why bother

- **DSP performance.** `DSPScript::process` runs Lua per audio block on the realtime
  thread (`src/scripting/dspscript.cpp`). LuaJIT's tracing JIT typically runs numeric
  Lua 5–50× faster than the 5.4 interpreter — directly widens what user DSP scripts can
  do.
- **FFI.** LuaJIT's `ffi` lets scripts declare C structs/arrays and call C functions with
  near-zero overhead and *no binding code*. This aligns exactly with the project
  direction of "don't bind every C++ class — thin C core + native Lua wrappers"
  (see [session-scripts.md](session-scripts.md)): buffer views, midi packing, and math
  helpers could become `ffi` cdata instead of sol2 usertypes / hand-written C modules
  (`bytes.c`, `vector.c`, `packed.h`).
- sol2 already supports LuaJIT (`SOL_LUAJIT`), so the binding layer itself is not a
  rewrite.

## The gaps

### Language: LuaJIT is Lua 5.1 (+ selected 5.2 features)

No integer subtype (everything is a double; 64-bit ints only via FFI/`ULL` suffixes), no
native bitwise operators (`&`, `|`, `~`, `<<` — use the `bit` library), no `//` floor
division, no `math.type`, no `utf8`, no `<close>`, no seedable per-state RNG semantics of
5.4. `goto` **is** supported (2.0+), and `LUAJIT_ENABLE_LUA52COMPAT` adds `goto`-adjacent
5.2 niceties (`table.pack/unpack` placement, `#` metamethod, etc.).

Initial audit of shipped scripts (`src/el/*.lua`, `scripts/*.lua`): no `math.type`,
`utf8`, `<close>`, or labels found — the shipped script corpus is close to 5.1-clean
already. Needs a proper pass for `//` and bitwise operators (not reliably greppable) and
for integer-vs-float formatting assumptions (`string.format('%d', ...)` on non-integral
doubles errors on 5.3+ but not 5.1 — and vice-versa gotchas exist).

**User impact:** any published promise that DSP scripts are "Lua 5.4" changes. If both
engines are supported, scripts must target the intersection (5.1 + bit library), which
should be documented in `docs/luastyle.md`.

### C API: LuaJIT exposes the 5.1 C API

Confirmed 5.2+/5.3+ API usage in-tree that would not compile against LuaJIT:
`lua_isinteger`, `lua_seti`/`lua_geti`, `luaL_setfuncs`, `luaL_requiref` (and friends) in
at least: `src/el/bytes.c`, `src/el/vector.c`, `src/el/MidiBuffer.cpp`,
`src/el/MidiMessage.cpp`, `src/el/AudioBufferImpl.ipp`, `src/scripting/dspscript.cpp`.

Standard fix: vendor the **compat-5.3** shim (lunarmodules/lua-compat-5.3), which
provides these as inline wrappers on 5.1/LuaJIT — the same headers compile unchanged
against 5.4. This is the established path and avoids forking our C modules.
`lua_isinteger` semantics remain approximate on LuaJIT (no true integers) — the modules
using it (`bytes.c` packing, midi byte handling) need case-by-case review, since byte
packing is exactly where double-vs-int64 differences bite.

### Platform / deployment

- **macOS arm64:** LuaJIT supports arm64, but JIT-compiled code needs `MAP_JIT` and the
  `com.apple.security.cs.allow-jit` entitlement under the hardened runtime. Fine for the
  Element app (we control entitlements).
- **Element as a plugin:** the plugin runs inside a *host* process (Live, Logic, ...)
  whose entitlements we do not control. A host without `allow-jit` breaks JIT — LuaJIT
  must run with the JIT disabled (`jit.off()` interpreter mode, still faster than PUC-Lua
  in many cases, but the headline win disappears). This is the single biggest strategic
  caveat: **the plugin builds may never reliably get JIT on macOS.**
- Windows/Linux: no equivalent restriction.

### Maintenance / ecosystem

LuaJIT is on a rolling v2.1 branch (actively maintained; OpenResty's fork is a fallback).
GC is the 5.1-era incremental collector — no 5.4 generational mode; for realtime use we
already should be (and with LuaJIT, must be) steering allocation-free `process()` paths
and explicit `collectgarbage('step')` scheduling on the message thread.

## Recommended shape of the work

Dual-engine behind a CMake option, not a hard cutover:

1. **`EL_LUA=lua54|luajit`** CMake option; FetchContent or vendor LuaJIT; define
   `SOL_LUAJIT` accordingly.
2. Vendor **compat-5.3** and switch the C modules/binding files listed above to include
   it (no-op for 5.4 builds).
3. Script corpus audit (`src/el/*.lua`, `scripts/*.lua`, docs examples) down to the
   5.1+bit intersection; update `docs/luastyle.md`.
4. CI test matrix: run the full `test_element` scripting suites under both engines —
   the Boost tests in `test/scripting/` become the compatibility gate for free.
5. Benchmark: a DSP-script stress fixture (existing test node pattern) timed under
   5.4 vs LuaJIT-interp vs LuaJIT-JIT, so the decision is data-driven.
6. Only after extensions/hooks land: explore FFI-based buffer/midi views as an
   alternative to `bytes.c`/`vector.c` (LuaJIT-only fast path, portable fallback kept).

## Sizing (rough)

| Step | Effort |
|---|---|
| CMake option + LuaJIT vendoring + sol2 config | small (days) |
| compat-5.3 adoption across C modules | small-medium; mechanical + `bytes.c` integer review |
| Script corpus 5.1-intersection audit | small |
| Dual-engine CI + benchmarks | medium |
| FFI buffer/midi fast path | larger, separate follow-up |

## Recommendation

Worth doing, **after** Phase 0 (session scripts/hooks) and the extension core, and as a
dual-engine option rather than a migration — because (a) the plugin-in-host entitlement
problem means 5.4 must remain a supported fallback anyway, and (b) the test suites those
phases add are exactly the safety net the second engine needs. The strategic win is FFI
plus DSP throughput in the standalone app; the cost is pinning the script dialect to the
5.1 intersection.
