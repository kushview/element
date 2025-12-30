// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/core.hpp>

namespace element {
namespace gzip {

/** Compresses and base64-encodes a string using gzip compression.
    
    This function takes an input string, compresses it using gzip compression,
    and then encodes the compressed data as base64. This is useful for storing
    or transmitting text data in a compact form.
    
    @param input  The string to compress and encode
    @return       A base64-encoded string containing the gzip-compressed input
    
    @see decode
*/
juce::String encode (const juce::String& input);

/** Decodes a base64-encoded gzip-compressed string.
    
    This function takes a base64-encoded string containing gzip-compressed data,
    decodes it from base64, decompresses it, and returns the original string.
    This is the inverse operation of encode().
    
    @param input  A base64-encoded string containing gzip-compressed data
    @return       The decompressed original string
    
    @see encode
*/
juce::String decode (const juce::String& input);

} // namespace gzip
} // namespace element
