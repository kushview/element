#pragma once

#include <element/juce/core.hpp>

namespace element {
namespace gzip {

using String = juce::String;
using InputStream = juce::GZIPDecompressorInputStream;
using OutputStream = juce::GZIPCompressorOutputStream;

/** GZip compress a string. */
inline static String compress (const String& string, int level = -1, int window = 0)
{
    juce::MemoryOutputStream mo;
    {
        OutputStream go (mo, level, window);
        go.writeString (string);
        go.flush();
    }
    return mo.toString();
}

/** GZip decompress a string. */
inline static String decompress (const String& string)
{
    auto size = string.getCharPointer().sizeInBytes();
    auto data = (void*) string.getCharPointer().getAddress();
    String out;
    {
        juce::MemoryInputStream mi (data, size, false);
        {
            InputStream go (mi);
            out = go.readEntireStreamAsString();
        }
    }

    return out;
}
} // namespace gzip
} // namespace element
