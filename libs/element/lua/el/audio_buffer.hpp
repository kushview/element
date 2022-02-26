
#pragma once

#include "lua-kv.hpp"
#include LKV_JUCE_HEADER

namespace kv {
namespace lua {

template<typename T>
inline juce::AudioBuffer<T>* 
create_audio_buffer (lua_State* L, int nchans, int nframes) {
    auto** userdata = lua_newuserdata (L, sizeof (juce::AudioBuffer<T>**));
    *userdata = new juce::AudioBuffer<T> (juce::jmax (0, nchans), juce::jmax (0, nframes));
    luaL_setmetatable (L, LKV_MT_AUDIO_BUFFER);
    return *userdata;
}

}}
