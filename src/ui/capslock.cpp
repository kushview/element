// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#if defined(__MINGW32__) || defined(_MSC_VER)
#include <windows.h>
#elif defined(__APPLE__)
#include <CoreGraphics/CoreGraphics.h>
#else
#warning "Caps lock detection not yet implemented on Linux"
#endif

#include "ui/capslock.hpp"

namespace element {

bool isCapsLockOn()
{
    bool result = false;
#if defined(__MINGW32__) || defined(_MSC_VER)
    result = (GetKeyState (VK_CAPITAL) & 0x0001) != 0;
#elif defined(__APPLE__)
    CGEventFlags flags = CGEventSourceFlagsState (kCGEventSourceStateHIDSystemState);
    result = (kCGEventFlagMaskAlphaShift & flags) != 0;
#else
    // linux
#endif
    return result;
}

} // namespace element
