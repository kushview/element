
#pragma once

#include "sol_helpers.hpp"
#include LKV_JUCE_HEADER

namespace element {
namespace lua {

template <typename T>
inline juce::AudioBuffer<T>*
    create_audio_buffer (lua_State* L, int nchans, int nframes)
{
    auto** userdata = lua_newuserdata (L, sizeof (juce::AudioBuffer<T>**));
    *userdata = new juce::AudioBuffer<T> (juce::jmax (0, nchans), juce::jmax (0, nframes));
    luaL_setmetatable (L, EL_MT_AUDIO_BUFFER);
    return *userdata;
}

} // namespace lua
} // namespace element
