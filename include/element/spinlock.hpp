// Copyright 2026 Kushview, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>

#include <element/element.h>

namespace element {

/** A simple spin lock using std::atomic. */
class EL_API SpinLock {
public:
    /** Initialize unlocked. */
    inline SpinLock() = default;
    inline ~SpinLock() = default;

    /** Lock or yield until locked. (non realtime) */
    void lock() const noexcept;
    /** Lock immediately or return false. (realtime)*/
    inline bool tryLock() const noexcept { return tryLock (0, 1); }
    /** Unlock the mutex. Note this does not check lock status. */
    inline void unlock() const noexcept { _lock = 0; }
    /** Returns true if the mutex is locked. */
    inline bool locked() const noexcept { return _lock.load() == 1; }

private:
    mutable std::atomic<int> _lock { 0 };
    /** @internal */
    inline bool tryLock (int c, int v) const noexcept
    {
        return _lock.compare_exchange_strong (c, v);
    }

    SpinLock (const SpinLock&) = delete;
    SpinLock& operator= (const SpinLock&) = delete;
    SpinLock (SpinLock&&) = delete;
    SpinLock& operator= (SpinLock&&) = delete;
};

} // namespace element
