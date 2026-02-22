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

#define ELEMENT_LOCAL_AUTH 1
#if ELEMENT_LOCAL_AUTH
inline constexpr const char* apiBaseEndpoint = "https://scratch-woo.local/wp-json/element-auth/v1";
inline constexpr const char* authorizeEndpoint = "https://scratch-woo.local/auth/authorize";
inline constexpr const char* tokenEndpoint = "https://scratch-woo.local/wp-json/element-auth/v1/token";
inline constexpr const char* refreshEndpoint = "https://scratch-woo.local/wp-json/element-auth/v1/token/refresh";
#else
/** Custom auth API base endpoint. */
inline constexpr const char* apiBaseEndpoint = "https://kushview.net/wp-json/element-auth/v1";

/** Browser authorization endpoint. */
inline constexpr const char* authorizeEndpoint = "https://kushview.net/auth/authorize";

/** Token exchange endpoint. */
inline constexpr const char* tokenEndpoint = "https://kushview.net/wp-json/element-auth/v1/token";

/** Refresh endpoint. */
inline constexpr const char* refreshEndpoint = "https://kushview.net/wp-json/element-auth/v1/token/refresh";

#endif

/** User settings key for persisted refresh token. */
inline constexpr const char* refreshTokenKey = "authRefreshToken";

/** User settings key for PKCE state. */
inline constexpr const char* pkceStateKey = "authPkceState";

/** User settings key for PKCE code verifier. */
inline constexpr const char* pkceVerifierKey = "authPkceVerifier";

/** Token response parsed from auth server payload. */
struct TokenResponse {
	bool success { false };
	juce::String accessToken;
	juce::String refreshToken;
	int expiresInSeconds { 0 };
	juce::String userDisplay;
	juce::String userEmail;
	juce::String error;
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

} // namespace element::auth
