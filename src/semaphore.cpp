// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include "semaphore.hpp"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

namespace element {

#ifdef __APPLE__

Semaphore::Semaphore()
{
    init (0);
}

Semaphore::Semaphore (unsigned initial)
{
    init (initial);
}

Semaphore::~Semaphore()
{
    destroy();
}

bool Semaphore::init (unsigned /* initial */)
{
    return semaphore_create (mach_task_self(), &semaphore, SYNC_POLICY_FIFO, 0)
               ? false
               : true;
}

void Semaphore::destroy()
{
    semaphore_destroy (mach_task_self(), semaphore);
}

void Semaphore::post()
{
    semaphore_signal (semaphore);
}

void Semaphore::wait()
{
    semaphore_wait (semaphore);
}

bool Semaphore::tryWait()
{
    const mach_timespec_t zero = { 0, 0 };
    return semaphore_timedwait (semaphore, zero) == KERN_SUCCESS;
}

#elif defined(_WIN32) || defined(_WIN64)

Semaphore::Semaphore()
{
    init (0);
}

bool Semaphore::init (unsigned initial)
{
    semaphore = CreateSemaphore (NULL, initial, LONG_MAX, NULL);
    return (semaphore) ? false : true;
}

Semaphore::~Semaphore()
{
    CloseHandle (semaphore);
}

void Semaphore::post()
{
    ReleaseSemaphore (semaphore, 1, NULL);
}

void Semaphore::wait()
{
    WaitForSingleObject (semaphore, INFINITE);
}

bool Semaphore::tryWait()
{
    return WAIT_FAILED != WaitForSingleObject (semaphore, 0);
}

#else /* !defined(__APPLE__) && !defined(_WIN32) */

Semaphore::Semaphore()
{
    init (0);
}

Semaphore::Semaphore (unsigned initial)
{
    init (initial);
}

bool Semaphore::init (unsigned initial)
{
    return sem_init (&semaphore, 0, initial) ? false : true;
}

Semaphore::~Semaphore()
{
    sem_destroy (&semaphore);
}

void Semaphore::post()
{
    sem_post (&semaphore);
}

void Semaphore::wait()
{
    /* Note that sem_wait always returns 0 in practice, except in
    gdb (at least), where it returns nonzero, so the while is
    necessary (and is the correct/safe solution in any case).
    */
    while (sem_wait (&semaphore) != 0)
    {
    }
}

bool Semaphore::tryWait()
{
    return (sem_trywait (&semaphore) == 0);
}

#endif

} // namespace element
