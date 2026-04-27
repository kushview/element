// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce/core.hpp>
#include <element/juce/data_structures.hpp>

namespace element {
class Settings;
}

namespace element::auth {

/** Public client ID used for desktop authorization flow. */
inline constexpr const char* clientId = "element-desktop";

/** Redirect URI registered for the desktop app callback. */
inline constexpr const char* redirectUri = "element://auth/callback";

#if ELEMENT_LOCAL_AUTH
inline constexpr const char* apiBaseEndpoint = "https://scratch-woo.local/wp-json/kv-auth/v1";
inline constexpr const char* authorizeEndpoint = "https://scratch-woo.local/auth/authorize";
inline constexpr const char* tokenEndpoint = "https://scratch-woo.local/wp-json/kv-auth/v1/token";
inline constexpr const char* refreshEndpoint = "https://scratch-woo.local/wp-json/kv-auth/v1/token/refresh";
#else
inline constexpr const char* apiBaseEndpoint = "https://kushview.net/wp-json/kv-auth/v1";
inline constexpr const char* authorizeEndpoint = "https://kushview.net/auth/authorize";
inline constexpr const char* tokenEndpoint = "https://kushview.net/wp-json/kv-auth/v1/token";
inline constexpr const char* refreshEndpoint = "https://kushview.net/wp-json/kv-auth/v1/token/refresh";
#endif

/** User settings key for persisted refresh token. */
inline constexpr const char* refreshTokenKey = "authRefreshToken";

/** User settings key for PKCE state. */
inline constexpr const char* pkceStateKey = "authPkceState";

/** User settings key for PKCE code verifier. */
inline constexpr const char* pkceVerifierKey = "authPkceVerifier";

/** User settings key for the cached signed appcast URL. */
inline constexpr const char* appcastUrlKey = "authAppcastUrl";

/** Token response parsed from auth server payload. */
struct TokenResponse
{
    bool success { false };
    juce::String accessToken;
    juce::String refreshToken;
    int expiresInSeconds { 0 };
    juce::String userDisplay;
    juce::String userEmail;
    juce::String error;

    /** Entitlement: whether the user has access to preview update builds. */
    bool previewUpdates { false };
};

/** Generates a random CSRF state value. */
juce::String generateState();

/** Generates a PKCE code verifier. */
juce::String generateCodeVerifier();

/** Builds the browser authorization URL for sign-in. */
juce::String buildAuthorizationURL (const juce::String& state,
                                    const juce::String& codeVerifier);

/** Stores pending PKCE values used by callback validation and token exchange. */
bool storePendingPKCE (element::Settings& settings,
                       const juce::String& state,
                       const juce::String& verifier);

/** Parses URL query key/value pairs from a callback URL string.

	@param urlString Full callback URL including query params
	@return Parsed key/value parameter array
 */
juce::StringPairArray parseQueryParameters (const juce::String& urlString);

/** Exchanges authorization code for access and refresh tokens.

	@param authCode Authorization code returned by OAuth callback
	@param codeVerifier PKCE verifier generated at authorization start
	@return Parsed token response and error details
 */
TokenResponse exchangeAuthorizationCode (const juce::String& authCode,
                                         const juce::String& codeVerifier);

/** Refreshes an access token using refresh token rotation semantics. */
TokenResponse refreshAccessToken (const juce::String& refreshToken);

/** Persists exchanged auth credentials into user settings.

	@param settings Application settings to persist credentials into
	@param tokenResponse Parsed OAuth token response data
 */
void persistTokens (element::Settings& settings, const TokenResponse& tokenResponse);

/** Gets and clears pending OAuth PKCE state and verifier values.

	@param settings Application settings where PKCE values were stored
	@param state Out value for state
	@param verifier Out value for code verifier
	@return true when both values were available
 */
bool consumePendingPKCE (element::Settings& settings, juce::String& state, juce::String& verifier);

/** Sends a best-effort revocation request for the given refresh token.

	This is a fire-and-forget call — failures are logged but not returned.
	Must be called from a background thread.

	@param refreshToken The opaque refresh token to revoke
 */
void revokeRefreshToken (const juce::String& refreshToken);

/** Fetches a short-lived signed appcast URL from the server.

	Uses the access token to authenticate. The returned URL is suitable for
	passing directly to Sparkle — no token header needed at fetch time.
	Must be called from a background thread.

	@param accessToken  The current JWT access token
	@return The signed URL string, or empty on failure
 */
juce::String fetchSignedAppcastUrl (const juce::String& accessToken);

/** Attempts to restore auth state on startup using a stored refresh token.

	If a refresh token is present in settings, exchanges it for a fresh token pair
	and persists the result. Must be called from a background thread.

	@param settings Application settings containing the stored refresh token
 */
void maybeRefreshOnStartup (element::Settings& settings);

/** Returns true if the cached signed appcast URL has expired or is empty.

	Parses the 'exp' query parameter embedded in the URL by the server.

	@param cachedUrl The URL string previously returned by fetchSignedAppcastUrl
 */
bool isAppcastUrlExpired (const juce::String& cachedUrl);

/** Begins the PKCE authorization flow.

	Generates a state token and code verifier, stores them for later validation,
	and returns the browser URL to open. Returns an empty string on failure.

	@param settings Application settings for PKCE state storage
	@return Authorization URL to open in the browser, or empty on failure
 */
juce::String beginAuthorizationFlow (element::Settings& settings);

/** Signs out the current user.

	Clears all stored auth credentials from settings and fires a best-effort
	server-side revocation of the refresh token on a background thread.

	@param settings Application settings containing the stored credentials
 */
void signOut (element::Settings& settings);

/** Handles an OAuth callback URL.

	Validates the PKCE state, exchanges the authorization code for tokens,
	and persists the result. Safe to call for any URL — non-callback URLs
	are silently ignored.

	@param urlString Full callback URL (e.g. element://auth/callback?code=...)
	@param settings  Application settings for PKCE state and token storage
 */
void handleCallback (const juce::String& urlString, element::Settings& settings);

} // namespace element::auth
