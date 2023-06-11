#pragma once

#include <element/juce/core.hpp>

namespace element {
namespace gzip {

using String = juce::String;
using InputStream = juce::GZIPDecompressorInputStream;
using OutputStream = juce::GZIPCompressorOutputStream;
using MemoryOutputStream = juce::MemoryOutputStream;
using MemoryInputStream = juce::MemoryInputStream;

inline static String encode (const String& input)
{
    MemoryOutputStream out;
    {
        MemoryOutputStream mo;
        {
            OutputStream gz (mo);
            gz.writeString (input);
        }
        juce::Base64::convertToBase64 (out, mo.getData(), mo.getDataSize());
    }
    String result; // = "data:application/gzip;base64, ";
    result << out.toString();
    return result;
}

inline static String decode (const String& input)
{
    MemoryOutputStream mo;
    juce::Base64::convertFromBase64 (mo, input);
    auto block = mo.getMemoryBlock();
    MemoryInputStream mi (block, false);
    InputStream dc (new MemoryInputStream (block, false),
                    true,
                    InputStream::zlibFormat,
                    mo.getDataSize());
    return dc.readString();
}

} // namespace gzip
} // namespace element
