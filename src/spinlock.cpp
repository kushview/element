// Copyright 2026 Kushview, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/spinlock.hpp>
#include "win32.hpp"

#if ! JUCE_WINDOWS
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
