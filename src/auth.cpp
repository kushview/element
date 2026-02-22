// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "auth.hpp"

#include <element/settings.hpp>
#include <cstring>

#if JUCE_MAC
    #include <CommonCrypto/CommonDigest.h>
#endif

namespace element::auth {
using namespace juce;

static String base64UrlEncode (const void* data, size_t numBytes)
{
    MemoryOutputStream output;
    Base64::convertToBase64 (output, data, numBytes);
    auto encoded = output.toString().trim();
    encoded = encoded.replaceCharacter ('+', '-')
                  .replaceCharacter ('/', '_')
                  .upToLastOccurrenceOf ("=", false, false);
    while (encoded.endsWithChar ('='))
        encoded = encoded.dropLastCharacters (1);
    return encoded;
}

static String createCodeChallenge (const String& verifier, String& method)
{
#if JUCE_MAC
    const auto utf8 = verifier.toUTF8();
    unsigned char digest[CC_SHA256_DIGEST_LENGTH] = { 0 };
    CC_SHA256 (reinterpret_cast<const unsigned char*> (utf8.getAddress()),
               static_cast<CC_LONG> (std::strlen (utf8.getAddress())),
               digest);
    method = "S256";
    return base64UrlEncode (digest, sizeof (digest));
#else
    method = "plain";
    return verifier;
#endif
}

static TokenResponse parseTokenPayload (const String& payload, const String& errorPrefix)
{
    TokenResponse response;

    const auto parsed = JSON::parse (payload);
    if (parsed.isVoid() || ! parsed.isObject())
    {
        response.error = errorPrefix + ": invalid JSON response";
        return response;
    }

    auto* object = parsed.getDynamicObject();
    if (object == nullptr)
    {
        response.error = errorPrefix + ": malformed response object";
        return response;
    }

    response.accessToken = object->getProperty ("access_token").toString().trim();
    response.refreshToken = object->getProperty ("refresh_token").toString().trim();
    response.expiresInSeconds = static_cast<int> (object->getProperty ("expires_in"));

    if (auto user = object->getProperty ("user"); user.isObject())
    {
        if (auto* userObj = user.getDynamicObject())
        {
            response.userEmail = userObj->getProperty ("email").toString().trim();
            response.userDisplay = userObj->getProperty ("display_name").toString().trim();
        }
    }

    if (response.userEmail.isEmpty())
        response.userEmail = object->getProperty ("email").toString().trim();
    if (response.userDisplay.isEmpty())
        response.userDisplay = object->getProperty ("display_name").toString().trim();
    if (response.userDisplay.isEmpty())
        response.userDisplay = response.userEmail;

    if (response.accessToken.isEmpty())
    {
        response.error = errorPrefix + ": access token missing in response";
        return response;
    }

    response.success = true;
    return response;
}

String generateState()
{
    return Uuid().toString().replaceCharacters ("{}-", "");
}

String generateCodeVerifier()
{
    static constexpr auto alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";
    static constexpr int alphabetLen = 66;
    static constexpr int verifierLength = 64;

    String verifier;
    verifier.preallocateBytes (verifierLength);

    auto& random = Random::getSystemRandom();
    for (int i = 0; i < verifierLength; ++i)
        verifier << String::charToString (alphabet[random.nextInt (alphabetLen)]);

    return verifier;
}

String buildAuthorizationURL (const String& state,
                              const String& codeVerifier)
{
    String challengeMethod;
    const auto codeChallenge = createCodeChallenge (codeVerifier, challengeMethod);

    String authUrl (authorizeEndpoint);
    authUrl << "?response_type=code"
            << "&client_id=" << URL::addEscapeChars (clientId, true)
            << "&redirect_uri=" << URL::addEscapeChars (redirectUri, true)
            << "&scope=" << URL::addEscapeChars ("basic", true)
            << "&state=" << URL::addEscapeChars (state, true)
            << "&code_challenge=" << URL::addEscapeChars (codeChallenge, true)
            << "&code_challenge_method=" << URL::addEscapeChars (challengeMethod, true);

    return authUrl;
}

bool storePendingPKCE (element::Settings& settings,
                       const String& state,
                       const String& verifier)
{
    if (auto* props = settings.getUserSettings())
    {
        props->setValue (pkceStateKey, state.trim());
        props->setValue (pkceVerifierKey, verifier.trim());
        return true;
    }

    return false;
}

StringPairArray parseQueryParameters (const String& urlString)
{
    StringPairArray parameters;
    const String query = urlString.fromFirstOccurrenceOf ("?", false, false);
    const StringArray pairs = StringArray::fromTokens (query, "&", "");
    for (const auto& pair : pairs)
    {
        const int equalsPos = pair.indexOfChar ('=');
        if (equalsPos <= 0)
            continue;

        const auto key = pair.substring (0, equalsPos);
        const auto value = URL::removeEscapeChars (pair.substring (equalsPos + 1));
        parameters.set (key, value);
    }

    return parameters;
}

TokenResponse exchangeAuthorizationCode (const String& authCode,
                                         const String& codeVerifier)
{
    TokenResponse response;
    if (authCode.isEmpty())
    {
        response.error = "Auth token exchange failed: missing authorization code";
        return response;
    }

    if (codeVerifier.isEmpty())
    {
        response.error = "Auth token exchange failed: missing PKCE code_verifier";
        return response;
    }

    auto body = String ("grant_type=authorization_code&code=");
    body << URL::addEscapeChars (authCode, true)
         << "&client_id=" << URL::addEscapeChars (clientId, true)
         << "&redirect_uri=" << URL::addEscapeChars (redirectUri, true)
         << "&code_verifier=" << URL::addEscapeChars (codeVerifier, true);

    URL tokenUrl (tokenEndpoint);
    tokenUrl = tokenUrl.withPOSTData (body);

    int statusCode = -1;
    StringPairArray responseHeaders;
    auto options = URL::InputStreamOptions (URL::ParameterHandling::inAddress)
                       .withHttpRequestCmd ("POST")
                       .withExtraHeaders ("Content-Type: application/x-www-form-urlencoded\r\n"
                                          "Accept: application/json\r\n")
                       .withConnectionTimeoutMs (15000)
                       .withStatusCode (&statusCode)
                       .withResponseHeaders (&responseHeaders);

    std::unique_ptr<InputStream> stream (tokenUrl.createInputStream (options));
    if (statusCode == -1 || stream == nullptr)
    {
        response.error = "Auth token exchange failed: unable to contact token endpoint";
        return response;
    }

    const auto payload = stream->readEntireStreamAsString().trim();

    if (statusCode < 200 || statusCode >= 300)
    {
        response.error = String ("Auth token exchange failed: HTTP ")
                         + String (statusCode)
                         + (payload.isNotEmpty() ? " " + payload : String());
        return response;
    }

    return parseTokenPayload (payload, "Auth token exchange failed");
}

TokenResponse refreshAccessToken (const String& refreshToken)
{
    TokenResponse response;
    if (refreshToken.isEmpty())
    {
        response.error = "Auth token refresh failed: missing refresh token";
        return response;
    }

    auto body = String ("grant_type=refresh_token&refresh_token=");
    body << URL::addEscapeChars (refreshToken, true)
         << "&client_id=" << URL::addEscapeChars (clientId, true);

    URL tokenUrl (refreshEndpoint);
    tokenUrl = tokenUrl.withPOSTData (body);

    int statusCode = -1;
    StringPairArray responseHeaders;
    auto options = URL::InputStreamOptions (URL::ParameterHandling::inAddress)
                       .withHttpRequestCmd ("POST")
                       .withExtraHeaders ("Content-Type: application/x-www-form-urlencoded\r\n"
                                          "Accept: application/json\r\n")
                       .withConnectionTimeoutMs (15000)
                       .withStatusCode (&statusCode)
                       .withResponseHeaders (&responseHeaders);

    std::unique_ptr<InputStream> stream (tokenUrl.createInputStream (options));
    if (statusCode == -1 || stream == nullptr)
    {
        response.error = "Auth token refresh failed: unable to contact token endpoint";
        return response;
    }

    const auto payload = stream->readEntireStreamAsString().trim();

    if (statusCode < 200 || statusCode >= 300)
    {
        response.error = String ("Auth token refresh failed: HTTP ")
                         + String (statusCode)
                         + (payload.isNotEmpty() ? " " + payload : String());
        return response;
    }

    return parseTokenPayload (payload, "Auth token refresh failed");
}

void persistTokens (element::Settings& settings, const TokenResponse& tokenResponse)
{
    if (! tokenResponse.success)
        return;

    settings.setUpdateKeyType ("member");
    settings.setUpdateKey (tokenResponse.accessToken);
    settings.setUpdateKeyUser (tokenResponse.userDisplay);

    if (auto* props = settings.getUserSettings())
        props->setValue (refreshTokenKey, tokenResponse.refreshToken);
}

bool consumePendingPKCE (element::Settings& settings, String& state, String& verifier)
{
    if (auto* props = settings.getUserSettings())
    {
        state = props->getValue (pkceStateKey).trim();
        verifier = props->getValue (pkceVerifierKey).trim();
        props->setValue (pkceStateKey, "");
        props->setValue (pkceVerifierKey, "");
        return state.isNotEmpty() && verifier.isNotEmpty();
    }

    return false;
}

} // namespace element::auth
