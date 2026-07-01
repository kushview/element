// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include "auth.hpp"

using namespace juce;
namespace auth = element::auth;

BOOST_AUTO_TEST_SUITE (AuthTests)

// RFC 7636 Appendix B PKCE test vector. This pins the S256 code_challenge to a
// value published by the spec (not merely "whatever our code emits"), so a
// per-platform regression in createCodeChallenge — e.g. the old macOS-only
// SHA-256 path that fell back to method=plain on Windows/Linux — fails here.
static const String kRfcVerifier = "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk";
static const String kRfcChallenge = "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM";

BOOST_AUTO_TEST_CASE (challenge_matches_rfc7636_vector)
{
    const auto url = auth::buildAuthorizationURL ("test-state", kRfcVerifier);
    const auto params = auth::parseQueryParameters (url);

    // The server strictly requires S256 on every platform.
    BOOST_REQUIRE_EQUAL (params["code_challenge_method"], String ("S256"));
    BOOST_REQUIRE_EQUAL (params["code_challenge"], kRfcChallenge);
}

BOOST_AUTO_TEST_CASE (authorization_url_carries_state_and_pkce_params)
{
    const auto state = auth::generateState();
    const auto verifier = auth::generateCodeVerifier();
    const auto url = auth::buildAuthorizationURL (state, verifier);
    const auto params = auth::parseQueryParameters (url);

    BOOST_REQUIRE_EQUAL (params["response_type"], String ("code"));
    BOOST_REQUIRE_EQUAL (params["state"], state);
    BOOST_REQUIRE_EQUAL (params["code_challenge_method"], String ("S256"));
    BOOST_REQUIRE (params["code_challenge"].isNotEmpty());

    // A real S256 digest is never the plaintext verifier (the old plain fallback).
    BOOST_REQUIRE (params["code_challenge"] != verifier);
}

BOOST_AUTO_TEST_CASE (challenge_is_base64url_without_padding)
{
    const auto url = auth::buildAuthorizationURL ("s", kRfcVerifier);
    const auto challenge = auth::parseQueryParameters (url)["code_challenge"];

    // base64url of a SHA-256 digest is 43 chars, unpadded, with -/_ substituted.
    BOOST_REQUIRE_EQUAL (challenge.length(), 43);
    for (int i = 0; i < challenge.length(); ++i)
    {
        const juce_wchar c = challenge[i];
        const bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
                        || (c >= '0' && c <= '9') || c == '-' || c == '_';
        BOOST_REQUIRE (ok);
    }
    BOOST_REQUIRE (! challenge.containsChar ('='));
    BOOST_REQUIRE (! challenge.containsChar ('+'));
    BOOST_REQUIRE (! challenge.containsChar ('/'));
}

BOOST_AUTO_TEST_CASE (code_verifier_meets_rfc_length_and_charset)
{
    // RFC 7636 requires the verifier to be 43-128 chars from the unreserved set.
    const auto verifier = auth::generateCodeVerifier();
    BOOST_REQUIRE (verifier.length() >= 43 && verifier.length() <= 128);

    for (int i = 0; i < verifier.length(); ++i)
    {
        const juce_wchar c = verifier[i];
        const bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
                        || (c >= '0' && c <= '9')
                        || c == '-' || c == '.' || c == '_' || c == '~';
        BOOST_REQUIRE (ok);
    }
}

BOOST_AUTO_TEST_SUITE_END()
