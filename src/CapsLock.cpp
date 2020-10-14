/*
    This file is part of Element
    Copyright (C) 2016-2020 Kushview, LLC.  All rights reserved.
    
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

#if defined(__MINGW32__) || defined(_MSC_VER)
 #include <Windows.h>
#elif defined(__APPLE__)
 #include <CoreGraphics/CoreGraphics.h>
#else
 #pragma warning "Linux not yet supported caps lock"
#endif

#include "CapsLock.h"

namespace Element {

bool isCapsLockOn()
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
