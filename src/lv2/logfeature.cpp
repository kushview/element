// Copyright 2014-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include "lv2/logfeature.hpp"

using namespace juce;

namespace element {
namespace LV2Callbacks {

int vprintf (LV2_Log_Handle handle, LV2_URID type, const char* fmt, va_list ap)
{
    // TODO: Lock
    return std::vfprintf (stderr, fmt, ap);
}

int printf (LV2_Log_Handle handle, LV2_URID type, const char* fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    const int ret = LV2Callbacks::vprintf (handle, type, fmt, args);
    va_end (args);
    return ret;
}
} // namespace LV2Callbacks

LogFeature::LogFeature()
{
    uri = LV2_LOG__log;
    feat.URI = uri.toRawUTF8();
    log.handle = this;
    log.printf = &LV2Callbacks::printf;
    log.vprintf = &LV2Callbacks::vprintf;
    feat.data = (void*) &log;
}

LogFeature::~LogFeature()
{
}

} // namespace element
