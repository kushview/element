// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/gzip.hpp>

namespace element {
namespace gzip {
using InputStream = juce::GZIPDecompressorInputStream;
using OutputStream = juce::GZIPCompressorOutputStream;

juce::String encode (const juce::String& input)
{
    juce::MemoryOutputStream out;
    {
        juce::MemoryOutputStream mo;
        {
            OutputStream gz (mo);
            gz.writeString (input);
        }
        juce::Base64::convertToBase64 (out, mo.getData(), mo.getDataSize());
    }
    // NOTE: "data:application/gzip;base64, ";
    return out.toString();
}

juce::String decode (const juce::String& input)
{
    juce::MemoryOutputStream mo;
    juce::Base64::convertFromBase64 (mo, input);
    auto block = mo.getMemoryBlock();
    juce::MemoryInputStream mi (block, false);
    InputStream dc (new juce::MemoryInputStream (block, false),
                    true,
                    InputStream::zlibFormat,
                    mo.getDataSize());
    return dc.readString();
}

} // namespace gzip
} // namespace element
