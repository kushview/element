// Copyright 2012 David Robillard <http://drobilla.net>
// SPDX-License-Identifier: ISC
// Adapted 2013 as C++ class by Michael Fisher <https://kushview.net>

#pragma once

#ifdef __APPLE__
#include <mach/mach.h>
typedef semaphore_t SemType;
#elif defined(_WIN32)
typedef void* SemType;
#else
#include <semaphore.h>
typedef sem_t SemType;
#endif

namespace element {

/**
   A counting semaphore.

   This is an integer that is always positive, and has two main operations:
   increment (post) and decrement (wait).  If a decrement can not be performed
   (i.e. the value is 0) the caller will be blocked until another thread posts
   and the operation can succeed.

   Semaphores can be created with any starting value, but typically this will
   be 0 so the semaphore can be used as a simple signal where each post
   corresponds to one wait.

   Semaphores are very efficient (much moreso than a mutex/cond pair).  In
   particular, at least on Linux, post is async-signal-safe, which means it
   does not block and will not be interrupted.  If you need to signal from
   a realtime thread, this is the most appropriate primitive to use.
*/
struct Semaphore
{
    /** Create a semaphore. */
    Semaphore();
    Semaphore (unsigned initial);

    ~Semaphore();

    bool init (unsigned initial);

    /** Destroy the semaphore */
    void destroy();

    /** Increment (and signal any waiters).
    Realtime safe. */
    void post();

    /** Wait until count is > 0 */
    void wait();

    /** Non-blocking version of wait().
    @return true if decrement was successful (lock was acquired). */
    bool tryWait();

private:
    SemType semaphore;
};

} // namespace element
