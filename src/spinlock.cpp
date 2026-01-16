// Copyright 2026 Kushview, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/spinlock.hpp>

#if _WIN32
#define WINDOWS_LWAN
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#undef min
#undef max
#else
#include <sched.h>
#endif

namespace element {

void SpinLock::lock() const noexcept
{
    if (tryLock())
        return;

    for (int i = 20; --i >= 0;)
        if (tryLock())
            return;

    while (! tryLock())
    {
#if _WIN32
        Sleep (0);
#else
        sched_yield();
#endif
    }
}

} // namespace element
