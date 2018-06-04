
/*
    Commands.h - This file is part of Element
    Copyright (C) 2016-2018 Kushview, LLC.  All rights reserved.
*/

#pragma once

namespace Element {

static inline bool isCapsLockOn()
{
   #if defined(__MINGW32__) || defined(_MSC_VER)
    return (GetKeyState (VK_CAPITAL) & 0x0001) != 0;
   #elif defined(__APPLE__)

   #else
        #pragma error "Linux not yet supported caps lock"
   #endif
}

}