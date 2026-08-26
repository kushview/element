// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/** Includes <windows.h> with WIN32_LEAN_AND_MEAN and NOMINMAX defined.

    Include this instead of <windows.h> directly so every translation unit
    gets the same configuration. It is a no-op on other platforms, so it can
    be included unconditionally.
*/

#include <juce_core/system/juce_TargetPlatform.h>

#if JUCE_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
