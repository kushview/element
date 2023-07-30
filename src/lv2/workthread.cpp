// Copyright 2014-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include "lv2/workthread.hpp"

using namespace juce;

#if _MSC_VER
#pragma warning(disable : 4127)
#endif

#if JUCE_DEBUG
#define WORKER_LOG(x) DBG (String ("worker: ") << x)
#else
#define WORKER_LOG(x)
#endif

namespace element {

WorkThread::WorkThread (const String& name, uint32_t bufsize, Thread::Priority priority)
    : Thread (name)
{
    nextWorkId = 0;
    bufferSize = (uint32_t) nextPowerOfTwo (bufsize);
    requests = std::make_unique<RingBuffer> (bufferSize);
    startThread (priority);
}

WorkThread::~WorkThread()
{
    doExit = true;
    signalThreadShouldExit();
    notify();
    waitForThreadToExit (100);
    requests = nullptr;
}

WorkerBase* WorkThread::getWorker (uint32_t workerId) const
{
    if (workerId == 0)
        return nullptr;

    for (int i = 0; i < workers.size(); ++i)
        if (workers.getUnchecked (i)->workId == workerId)
            return workers.getUnchecked (i);

    return nullptr;
}

void WorkThread::addWorker (WorkerBase* worker)
{
    worker->workId = ++nextWorkId;
    WORKER_LOG (getThreadName() + " registering worker: " + String (worker->workId));
    workers.addIfNotAlreadyThere (worker);
}

void WorkThread::removeWorker (WorkerBase* worker)
{
    WORKER_LOG (getThreadName() + " removing worker: " + String (worker->workId));
    workers.removeFirstMatchingValue (worker);
    worker->workId = 0;
}

void WorkThread::run()
{
    HeapBlock<uint8> buffer;
    int32 readBufferSize = 0;

    while (true)
    {
        this->wait (-1);

        if (doExit || threadShouldExit())
            break;

        while (! validateMessage (*requests))
            Thread::sleep (6);

        if (doExit || threadShouldExit())
            break;

        uint32_t size = 0;
        if (requests->read (&size, sizeof (size)) < sizeof (size))
        {
            WORKER_LOG ("error reading request: message size");
            continue;
        }

        uint32_t workId;
        if (requests->read (&workId, sizeof (workId)) < sizeof (workId))
        {
            WORKER_LOG ("error reading request: worker id");
            continue;
        }

        if (workId == 0)
            continue;

        if (size > static_cast<uint32_t> (readBufferSize))
        {
            readBufferSize = nextPowerOfTwo (size);
            buffer.realloc (readBufferSize);
        }

        if (requests->read (buffer.getData(), size) < size)
        {
            WORKER_LOG ("error reading request: message body");
            continue;
        }

        {
            if (WorkerBase* const worker = getWorker (workId))
            {
                while (! worker->flag.setWorking (true))
                {
                }
                worker->processRequest (size, buffer.getData());
                while (! worker->flag.setWorking (false))
                {
                }
            }
        }

        if (threadShouldExit() || doExit)
            break;
    }

    buffer.free();
}

bool WorkThread::scheduleWork (WorkerBase* worker, uint32_t size, const void* data)
{
    jassert (size > 0 && worker && worker->workId != 0);
    if (! requests->canWrite (getRequiredSpace (size)))
        return false;

    if (requests->write (&size, sizeof (size)) < sizeof (uint32_t))
        return false;

    if (requests->write (&worker->workId, sizeof (worker->workId)) < sizeof (worker->workId))
        return false;

    if (requests->write (data, size) < size)
        return false;

    notify();
    return true;
}

bool WorkThread::validateMessage (RingBuffer& ring)
{
    uint32_t size = 0;
    ring.peak (&size, sizeof (size));
    return ring.canRead (getRequiredSpace (size));
}

WorkerBase::WorkerBase (WorkThread& thread, uint32_t bufsize)
    : owner (thread)
{
    responses = std::make_unique<RingBuffer> (bufsize);
    response.calloc (bufsize);
    thread.addWorker (this);
}

WorkerBase::~WorkerBase()
{
    while (flag.isWorking())
    {
        Thread::sleep (100);
    }

    owner.removeWorker (this);
    responses = nullptr;
    response.free();
}

bool WorkerBase::scheduleWork (uint32_t size, const void* data)
{
    return owner.scheduleWork (this, size, data);
}

bool WorkerBase::respondToWork (uint32_t size, const void* data)
{
    if (! responses->canWrite (sizeof (size) + size))
        return false;

    if (responses->write (&size, sizeof (size)) < sizeof (size))
        return false;

    if (responses->write (data, size) < size)
        return false;

    return true;
}

void WorkerBase::processWorkResponses()
{
    uint32_t remaining = responses->getReadSpace();
    uint32_t size = 0;

    while (remaining >= sizeof (uint32_t))
    {
        /* respond next cycle if response isn't ready */
        if (! validateMessage (*responses))
            return;

        responses->read (&size, sizeof (size));
        responses->read (response.getData(), size);
        processResponse (size, response.getData());
        remaining -= (sizeof (uint32_t) + size);
    }
}

bool WorkerBase::validateMessage (RingBuffer& ring)
{
    // the worker only validates message size
    uint32_t size = 0;
    ring.peak (&size, sizeof (size));
    return ring.canRead (size + sizeof (size));
}

void WorkerBase::setSize (uint32_t newSize)
{
    responses = std::make_unique<RingBuffer> (newSize);
    response.realloc (newSize);
}

} // namespace element
