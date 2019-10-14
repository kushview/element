
/*
    Commands.h - This file is part of Element
    Copyright (C) 2016-2018 Kushview, LLC.  All rights reserved.
*/

#pragma once

#if defined(__MINGW32__) || defined(_MSC_VER)
 #include <Windows.h>
#elif defined(__APPLE__)
 #include <CoreGraphics/CoreGraphics.h>
#else
 #pragma warning "Linux not yet supported caps lock"
#endif

namespace Element {

static inline bool isCapsLockOn()
{
   #if defined(__MINGW32__) || defined(_MSC_VER)
    return (GetKeyState (VK_CAPITAL) & 0x0001) != 0;
   #elif defined(__APPLE__)
    CGEventFlags flags = CGEventSourceFlagsState (kCGEventSourceStateHIDSystemState);
    return (kCGEventFlagMaskAlphaShift & flags) != 0;
   #else
    // linux
   #endif
    return false;
}

}
