/*
    This file is part of Element
    Copyright (C) 2019-2021  Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "utils.hpp"
#include "engine/nodes/BaseProcessor.h"

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
#if JUCE_PLUGINHOST_LV2
        fmts.add ("LV2");
#endif

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
