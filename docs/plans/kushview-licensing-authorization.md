# Kushview Licensing & Authorization Architecture

**Status:** Draft / design
**Scope:** Kushview-owned plugins only. Client-side design complete; server-side deferred (stubbed contracts only).
**Goal:** A subscriber (or standalone purchaser) authenticates once, and every Kushview plugin they're entitled to unlocks silently — no per-plugin user/pass prompt. Cancelling a subscription revokes access on the next refresh.

---

## 1. Summary

Introduce a dedicated **Kushview Account Manager** application as the single place that logs in and manages entitlements. It writes a **shared per-user store** of auth tokens and signed unlock keys. Element and every Kushview plugin link a small **shared read-side SDK** that reads that store and applies keys through a **common JUCE product-unlocking implementation** — never running an interactive login themselves.

This mirrors the industry-standard pattern (iLok License Manager, NI Native Access, Waves Central, Steinberg Download Assistant): one privileged writer, many trivial readers.

### Why this shape

- **Plugins stay trivial.** No OAuth, no browser, no URL-scheme handling shipped in each plugin — just "read store → apply key → else fall back to the existing manual prompt." Less code, less attack surface, replicated across every product.
- **Auth changes ship once**, in the Manager, decoupled from plugin release cycles.
- **Background subscription refresh.** Subscription keys are short-lived by design; a plugin can only refresh while loaded. The Manager runs as a login-item/agent and keeps every owned plugin's key fresh regardless of whether anything is open — the one place this can be solved cleanly.
- **Graceful degradation.** No token / offline / non-subscriber → plugin shows today's manual unlock UI. Nothing breaks.

---

## 2. Background: JUCE unlocking primer (for the target project)

Kushview plugins are JUCE products. JUCE's licensing primitives:

- **RSA key pair.** Generated once per product with `juce::RSAKey::createKeyPair()`.
  - **Private key** signs unlock "key files". **Lives only on the server.** Never shipped, never in any client.
  - **Public key** is compiled into the product and only *verifies* a signed key file. It cannot forge licenses. It is safe to embed — and is already inside every shipped plugin binary.
- **Key file.** A signed blob (produced server-side by `juce::KeyGeneration::generateKeyFile(...)`) containing the user email, product ID, **machine IDs**, and an **expiry**. Applied client-side with `OnlineUnlockStatus::applyKeyFile(...)`.
- **`juce::OnlineUnlockStatus`.** The client-side state object. We use it in "key file" mode (`applyKeyFile` / `isUnlocked` / `getExpiryTime`) and **bypass its built-in web-authentication flow** entirely — keys come from the shared store, not from OnlineUnlockStatus's own HTTP calls.

Two protections do the real work: **machine-locking** (a key file only unlocks on the machines whose IDs it was signed for) and **expiry** (subscription semantics). OnlineUnlockStatus's own state obfuscation is weak and is *not* relied on for security.

### Current state in Element (starting point)

Element already has the auth foundation this builds on:

- OAuth2 Authorization-Code + PKCE against WordPress `kv-auth/v1` (`src/auth.cpp`, `src/auth.hpp`, namespace `element::auth`).
- Token response already carries an `entitlements` object (today just `preview_updates`) plus user email/display name.
- An established precedent for "authenticated request → short-lived server-signed artifact": `/appcast-url?plat=` returns a signed, expiring Sparkle feed URL that the client caches and refreshes when expired (`GuiService::checkUpdates`, `auth::isAppcastUrlExpired`).
- **No** `OnlineUnlockStatus`/`RSAKey`/`KeyGeneration` anywhere yet — the unlock layer is net-new.
- Plugin scanning/hosting has no entitlement gate — nothing to unwind.

The signed-key endpoint is the same shape as the existing appcast-url endpoint. The OAuth/PKCE client is the reusable core that gets lifted into the shared SDK.

---

## 3. Component overview

```
┌───────────────────────────────────────────────┐
│           Kushview Account Manager             │   the ONLY app that logs in
│  • interactive OAuth2 + PKCE (browser)         │
│  • URL-scheme callback (kushview://)           │
│  • entitlement sync (tier → product list)      │
│  • key fetch + refresh daemon                  │
│  • (future) product download / install         │
└───────────────────────┬───────────────────────┘
                        │ writes (atomic)
                        ▼
        ╔═══════════════════════════════════════╗
        ║   Shared per-user Kushview store       ║
        ║   • tokens (OS secure storage)         ║
        ║   • keys/<productId>.key (machine-     ║
        ║     locked, expiring signed blobs)     ║
        ║   • entitlements.json                  ║
        ╚═══════════════════════════════════════╝
                        ▲                ▲
                read    │                │  read
        ┌───────────────┴──┐        ┌────┴──────────────────┐
        │     Element      │        │   Kushview plugins    │
        │  (host + reader) │        │  ToneGenerator, …     │
        └──────────────────┘        └───────────────────────┘
              both link the shared READ-SIDE SDK:
              KushviewUnlockStatus + StoreReader + fallback
```

### 3.1 Kushview Account Manager (new application)

The single privileged writer.

Responsibilities:
- Interactive **OAuth2 + PKCE** login (lifted from `element::auth`), including browser launch and `kushview://auth/callback` URL-scheme handling.
- Fetch and persist **tokens** into the shared store (refresh-token rotation, silent refresh).
- Fetch **entitlements** (tier → owned/subscribed product list) and persist to `entitlements.json`.
- For each entitled product, request a **signed, machine-locked, expiring key file** and write it to `keys/<productId>.key`.
- **Refresh daemon:** run on login-item/agent schedule; re-request any key nearing expiry so subscription keys never lapse silently. On the server saying "no longer entitled," delete that product's key.
- Account UI: signed-in identity, tier, owned products, per-product authorization status, manual "refresh all."
- (Future) download & install products and updates; can absorb Element's appcast entitlement flow.

Cross-platform: one codebase (JUCE app or native), signed/notarized, self-updating.

### 3.2 Shared read-side SDK (linked by Element + every plugin)

A small static/shared library. Contains **no interactive login** — read + apply + fall back only.

Public surface:

```cpp
namespace kv::lic {

// Identity a product provides about itself.
struct ProductInfo {
    juce::String productId;      // stable slug, e.g. "kv.tonegenerator"
    juce::String publicKey;      // this product's embedded RSA public key
    juce::String displayName;    // for any UI/prompt fallback
};

// Locates and reads the shared per-user store. Read-only.
class StoreReader {
public:
    static juce::File storeDir();                         // platform-specific (see §5)
    juce::File keyFileFor (const juce::String& productId) const;
    bool hasTokens() const;                               // is anyone logged in?
    // NOTE: reads tokens for refresh-on-read only if we choose to allow it (see §7, open decision).
};

// Common JUCE product-unlocking implementation. One per plugin instance.
class KushviewUnlockStatus : public juce::OnlineUnlockStatus {
public:
    explicit KushviewUnlockStatus (ProductInfo);

    // The unlock ladder (see §4.2). Cheap; safe to call on load.
    bool authorize();

    juce::String getProductID() override        { return info.productId; }
    juce::RSAKey  getPublicKey() override        { return juce::RSAKey (info.publicKey); }
    juce::String getState() override;            // persisted applied-key state
    void         saveState (const juce::String&) override;
    // Built-in web auth is intentionally unused:
    juce::URL    getServerAuthenticationURL() override { return {}; }
    juce::String readReplyFromWebserver (const juce::String&, const juce::String&) override { return {}; }

private:
    ProductInfo info;
    StoreReader store;
};

} // namespace kv::lic
```

### 3.3 Common key-writing pattern (the shared contract)

This is the "common pattern for key writing" — one implementation, honored identically by the Manager (writer) and every plugin (reader).

- **File per product:** `keys/<productId>.key`, containing the raw JUCE key-file text produced by `KeyGeneration::generateKeyFile` server-side.
- **Atomic writes** (Manager): write to `keys/<productId>.key.tmp`, `fsync`, rename over the target. Readers never observe a half-written file.
- **Applied-state cache** (plugin): after `applyKeyFile` succeeds, the plugin persists the applied state via `saveState()` into its *own* settings. So an already-authorized plugin keeps working even if the store file is later removed — until the embedded **expiry** lapses.
- **Machine-locked:** every key file carries this machine's IDs; copying it to another machine fails `applyKeyFile`. This is what makes the shared store safe to sync/back-up.
- **Expiry-bearing:** subscription keys are short-lived (e.g. 30 days). The Manager refreshes ahead of expiry; a lapsed entitlement simply stops being refreshed and the plugin falls back to prompting after expiry.

### 3.4 Per-plugin integration (minimal)

Each plugin adds only:

```cpp
static const kv::lic::ProductInfo kToneGenProduct {
    "kv.tonegenerator",
    EMBEDDED_TONEGEN_PUBLIC_KEY,   // build-time constant, safe to embed
    "Kushview Tone Generator"
};

// On construction / first UI show:
kv::lic::KushviewUnlockStatus unlock (kToneGenProduct);
if (! unlock.authorize())
    showManualUnlockPrompt();      // existing per-plugin fallback UI
```

Nothing else. No OAuth, no browser, no URL scheme.

### 3.5 Element's role

Element is just another reader of the shared store for the *plugin* keys. Its existing OAuth login for **update entitlements** continues to work as-is short-term.

- **Short-term:** Element keeps its own login (writes the same shared token store the Manager uses) and additionally reads plugin keys via the SDK. Both apps interoperate through one store.
- **Long-term:** the Manager becomes the canonical account/entitlement hub; Element migrates to a pure reader and drops its embedded login UI. *(Decision — see §7.)*

---

## 4. Data flows

### 4.1 Login (Manager only)

```
User clicks Sign In (Manager)
  → PKCE verifier/challenge generated, state stored
  → browser opens kushview.net/auth/authorize
  → user authenticates on the store
  → redirect kushview://auth/callback?code=…&state=…
  → OS routes to Manager URL-scheme handler
  → validate state, exchange code at kv-auth/v1/token
  → receive JWT access token + refresh token + entitlements{tier, products[]}
  → write tokens (OS secure storage) + entitlements.json
  → for each entitled product: fetch signed key → write keys/<productId>.key
```

### 4.2 Unlock ladder (Element + plugins, on load)

```
authorize():
  1. Local applied state valid AND not expired?        → unlocked, ZERO network
  2. keys/<productId>.key present?
        applyKeyFile(blob)
          machine matches AND not expired?             → unlocked, cache state
  3. Otherwise                                          → return false
        → caller shows existing manual unlock prompt
```

Step 1 makes the common case free (no I/O beyond a settings read). Step 3 guarantees graceful degradation.

### 4.3 Background subscription refresh (Manager daemon)

```
On schedule / login-item wake:
  ensure access token fresh (silent refresh; rotate refresh token)
  GET entitlements
  for each product:
      if entitled:
          if key missing OR expiry within threshold (e.g. < 7 days):
              fetch new signed, machine-locked key → atomic write
      else:
          delete keys/<productId>.key   # revoked / downgraded
```

This is the mechanism that gives real subscription semantics: cancel → next refresh stops re-signing → key expires → plugin falls back to prompting.

---

## 5. Shared store layout & locations

```
<KushviewAccountDir>/
  tokens          → refresh/access tokens (prefer OS secure storage, see below)
  entitlements.json
  keys/
    kv.tonegenerator.key
    kv.<product>.key
```

Platform base directory:
- **macOS:** `~/Library/Application Support/Kushview/Account/`
- **Windows:** `%APPDATA%\Kushview\Account\`
- **Linux:** `$XDG_CONFIG_HOME/kushview/account/` (fallback `~/.config/kushview/account/`)

**Token at-rest protection:** store the refresh token in OS secure storage where available — macOS **Keychain**, Windows **Credential Manager (DPAPI)**, Linux **libsecret** — falling back to a restricted-permission file (0600) only if unavailable. Key files themselves are machine-locked, so they are low-sensitivity and can live as plain files.

---

## 6. Security model & invariants

Non-negotiable:

1. **Private signing key never leaves the server.** All key-file generation is server-side. No client (Manager, Element, plugin) can mint or re-sign keys.
2. **Public keys are embedded and harmless.** They only verify. Already present in shipped binaries.
3. **Entitlement decisions are server-side.** The client asks; the server returns a signed key or a denial. A patched client cannot grant itself products — it can only replay what the server signed, which is machine-locked and time-limited.
4. **Machine-locked keys.** Every key file is bound to `getLocalMachineIDs()`; it cannot be lifted to another machine.
5. **Expiring keys.** Subscriptions rely on short TTL + refresh. No perpetual key for subscription tiers.
6. **TLS everywhere**; tokens in OS secure storage.
7. **Fail closed to the *prompt*, not to unlocked.** Any failure in the ladder ends at the manual unlock UI, never at an unearned unlock.

Threats explicitly out of scope of "safe embedding": binary patching to skip the `isUnlocked()` check is possible for any offline-verifiable scheme and is not made worse by this design; machine-locking + expiry + server-side entitlement are the mitigations.

---

## 7. Open decisions

| # | Decision | Options | Lean |
|---|----------|---------|------|
| 1 | Element long-term auth | (a) keep own login + read keys; (b) delegate all account to Manager, Element becomes pure reader | (a) now → (b) later |
| 2 | Can plugins refresh a token themselves? | Read-only (Manager is sole refresher) vs. plugins allowed silent token refresh when store token is stale | Read-only first; simplest, smallest attack surface |
| 3 | Key TTL & refresh threshold | e.g. 30-day TTL, refresh < 7 days | confirm with subscription cadence |
| 4 | Store token format | OS secure storage vs. encrypted file | OS secure storage, file fallback |
| 5 | Standalone-plugin-without-Manager UX | require Manager once vs. per-plugin lightweight login | require Manager (matches Native Access) |

---

## 8. Server-side (DEFERRED — contract stubs only)

Not designed here; captured so the client contracts are unambiguous. Extends the existing `kv-auth/v1` namespace.

- `GET /kv-auth/v1/entitlements` (Bearer) → `{ tier, products: [productId…] }`
- `GET /kv-auth/v1/plugin-key?product=<id>&machine=<ids>` (Bearer)
  → signed JUCE key file (machine-locked, expiring) or `403` if not entitled.
  Same shape as the existing `/appcast-url` precedent.
- Reuse existing `/token`, `/token/refresh`, `/token/revoke`.

Signing service holds the per-product **private** keys and calls `KeyGeneration::generateKeyFile`.

---

## 9. Phased roadmap

1. **Shared read-side SDK** — `StoreReader`, `KushviewUnlockStatus`, key-file read/apply/cache, store layout & locations. Unit-testable with a fixture store and a locally generated key pair.
2. **Per-plugin integration in one product** (Tone Generator) — embed public key, wire the ladder + fallback prompt. Prove silent unlock from a hand-placed key file.
3. **Account Manager MVP** — lift `element::auth` OAuth/PKCE into it, URL-scheme handling, write tokens + entitlements + keys to the store.
4. **Refresh daemon** — login-item/agent, expiry-driven re-fetch, revocation delete.
5. **Element as reader** — link the SDK, read plugin keys (keep existing update login).
6. **Roll SDK across remaining plugins.**
7. **Server-side** — entitlements + plugin-key signing endpoints (separate plan).
8. *(Future)* download/install + migrate update entitlement into Manager.

---

## 10. Portability notes (for the other project)

- The SDK and `KushviewUnlockStatus` are pure JUCE + a store path — no Element dependencies. They drop into any Kushview JUCE product.
- The only per-product inputs are the **productId**, the **embedded public key**, and a **display name**.
- The store layout in §5 and the key-writing pattern in §3.3 are the interop contract between the Manager and all readers — keep them identical on both sides.
- Nothing here requires the server work to exist first: with a locally generated test key pair and a hand-written `keys/<id>.key`, the entire read/apply/ladder path is buildable and testable today.
