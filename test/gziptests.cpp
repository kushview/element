// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <boost/test/unit_test.hpp>
#include <element/gzip.hpp>

using element::gzip::decode;
using element::gzip::encode;
using juce::String;

BOOST_AUTO_TEST_SUITE (GzipTests)

BOOST_AUTO_TEST_CASE (encode_simple_string)
{
    String input = "Hello, World!";
    String encoded = encode (input);
    
    BOOST_REQUIRE (encoded.isNotEmpty());
    BOOST_REQUIRE (encoded != input); // Should be different after encoding
}

BOOST_AUTO_TEST_CASE (decode_simple_string)
{
    String input = "Hello, World!";
    String encoded = encode (input);
    String decoded = decode (encoded);
    
    BOOST_REQUIRE_EQUAL (decoded, input);
}

BOOST_AUTO_TEST_CASE (encode_decode_roundtrip)
{
    String input = "This is a test string for gzip compression.";
    String encoded = encode (input);
    String decoded = decode (encoded);
    
    BOOST_REQUIRE_EQUAL (decoded, input);
}

BOOST_AUTO_TEST_CASE (encode_empty_string)
{
    String input = "";
    String encoded = encode (input);
    String decoded = decode (encoded);
    
    BOOST_REQUIRE_EQUAL (decoded, input);
}

BOOST_AUTO_TEST_CASE (encode_multiline_text)
{
    String input = "Line 1\nLine 2\nLine 3\n";
    String encoded = encode (input);
    String decoded = decode (encoded);
    
    BOOST_REQUIRE_EQUAL (decoded, input);
}

BOOST_AUTO_TEST_CASE (encode_special_characters)
{
    String input = "Special chars: !@#$%^&*()_+-=[]{}|;':\",./<>?";
    String encoded = encode (input);
    String decoded = decode (encoded);
    
    BOOST_REQUIRE_EQUAL (decoded, input);
}

BOOST_AUTO_TEST_CASE (encode_unicode_text)
{
    String input = "Unicode: \u00E9\u00F1\u00FC \u4E2D\u6587 \u0440\u0443\u0441\u0441\u043A\u0438\u0439";
    String encoded = encode (input);
    String decoded = decode (encoded);
    
    BOOST_REQUIRE_EQUAL (decoded, input);
}

BOOST_AUTO_TEST_CASE (encode_large_text)
{
    // Create a larger string to test compression
    String input;
    for (int i = 0; i < 1000; ++i)
    {
        input << "This is line " << String (i) << " of the test data.\n";
    }
    
    String encoded = encode (input);
    String decoded = decode (encoded);
    
    BOOST_REQUIRE_EQUAL (decoded, input);
    
    // Verify that compression actually reduces size for repetitive data
    // The encoded size should be smaller than base64 of raw data
    BOOST_REQUIRE (encoded.length() < input.length());
}

BOOST_AUTO_TEST_CASE (encode_repeated_content)
{
    // Highly repetitive content should compress well
    String input;
    for (int i = 0; i < 100; ++i)
    {
        input << "AAAAAAAAAA";
    }
    
    String encoded = encode (input);
    String decoded = decode (encoded);
    
    BOOST_REQUIRE_EQUAL (decoded, input);
    BOOST_REQUIRE (encoded.length() < input.length());
}

BOOST_AUTO_TEST_CASE (encode_whitespace)
{
    String input = "   \t\n\r   ";
    String encoded = encode (input);
    String decoded = decode (encoded);
    
    BOOST_REQUIRE_EQUAL (decoded, input);
}

BOOST_AUTO_TEST_CASE (encode_single_character)
{
    String input = "A";
    String encoded = encode (input);
    String decoded = decode (encoded);
    
    BOOST_REQUIRE_EQUAL (decoded, input);
}

BOOST_AUTO_TEST_CASE (encode_numbers_and_symbols)
{
    String input = "1234567890 !@#$%^&*() \n\t\r";
    String encoded = encode (input);
    String decoded = decode (encoded);
    
    BOOST_REQUIRE_EQUAL (decoded, input);
}

BOOST_AUTO_TEST_CASE (encode_json_like_structure)
{
    String input = R"({"key": "value", "number": 42, "array": [1, 2, 3]})";
    String encoded = encode (input);
    String decoded = decode (encoded);
    
    BOOST_REQUIRE_EQUAL (decoded, input);
}

BOOST_AUTO_TEST_CASE (encode_xml_like_structure)
{
    String input = "<?xml version=\"1.0\"?>\n<root>\n  <element>value</element>\n</root>";
    String encoded = encode (input);
    String decoded = decode (encoded);
    
    BOOST_REQUIRE_EQUAL (decoded, input);
}

BOOST_AUTO_TEST_CASE (encode_produces_base64)
{
    String input = "Test string";
    String encoded = encode (input);
    
    // Encoded output should be valid base64 (only contains base64 chars)
    for (int i = 0; i < encoded.length(); ++i)
    {
        juce::juce_wchar c = encoded[i];
        bool isBase64 = (c >= 'A' && c <= 'Z') || 
                       (c >= 'a' && c <= 'z') || 
                       (c >= '0' && c <= '9') || 
                       c == '+' || c == '/' || c == '=' || 
                       c == '\n' || c == '\r'; // base64 can have line breaks
        BOOST_REQUIRE (isBase64);
    }
}

BOOST_AUTO_TEST_CASE (multiple_encode_decode_cycles)
{
    String input = "Test multiple cycles";
    
    // Should be able to encode and decode multiple times
    String result = input;
    for (int i = 0; i < 5; ++i)
    {
        result = encode (result);
    }
    
    for (int i = 0; i < 5; ++i)
    {
        result = decode (result);
    }
    
    BOOST_REQUIRE_EQUAL (result, input);
}

BOOST_AUTO_TEST_SUITE_END()
