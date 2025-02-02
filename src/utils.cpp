// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include "utils.hpp"

#if JUCE_WINDOWS
#include <windows.h>
#endif

namespace element {
namespace Util {

StringArray getSupportedAudioPluginFormats()
{
    StringArray fmts;

#if JUCE_MAC && JUCE_PLUGINHOST_AU
    fmts.add ("AudioUnit");
#endif
#if JUCE_PLUGINHOST_VST
    fmts.add ("VST");
#endif
#if JUCE_PLUGINHOST_VST3
    fmts.add ("VST3");
#endif
#if JUCE_PLUGINHOST_LADSPA
    fmts.add ("LADSPA");
#endif

    fmts.add ("LV2");
    fmts.add ("CLAP");
    fmts.sort (false);
    return fmts;
}

bool isRunningInWine()
{
#if JUCE_WINDOWS
    HMODULE ntdll = GetModuleHandleA ("ntdll");
    return ntdll != nullptr && GetProcAddress (ntdll, "wine_get_version") != nullptr;
#else
    return false;
#endif
}

} // namespace Util
} // namespace element
