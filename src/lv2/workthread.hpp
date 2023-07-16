/*
    Copyright (c) 2014-2019  Michael Fisher <mfisher@kushview.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

#include <cstdint>

#include <element/juce/core.hpp>

#include "ringbuffer.hpp"

namespace element {

class WorkerBase;

/** A worker thread
    Capable of scheduling non-realtime work from a realtime context.
 */
class WorkThread : public juce::Thread
{
public:
    WorkThread (const juce::String& name, uint32_t bufsize, int32 priority = 5);
    ~WorkThread();

    inline static uint32_t getRequiredSpace (uint32_t msgSize) { return msgSize + (2 * sizeof (uint32_t)); }

protected:
    friend class WorkerBase;

    /** Register a worker for scheduling. Does not take ownership */
    void addWorker (WorkerBase* worker);

    /** Deregister a worker from scheduling. Does not delete the worker */
    void removeWorker (WorkerBase* worker);

    /** Schedule non-realtime work
        Workers will call this in Worker::scheduleWork */
    bool scheduleWork (WorkerBase* worker, uint32_t size, const void* data);

private:
    uint32_t bufferSize;

    WorkerBase* getWorker (uint32_t workerId) const;
    juce::Array<WorkerBase*, juce::CriticalSection> workers;

    uint32_t nextWorkId;
    bool doExit = false;

    std::unique_ptr<RingBuffer> requests; ///< requests to process

    /** @internal Validate a ringbuffer for message completeness */
    bool validateMessage (RingBuffer& ring);

    /** @internal The work thread function */
    void run();
};

/** A flag that indicates whether work is happening or not */
class WorkFlag
{
public:
    WorkFlag() { flag = 0; }
    inline bool isWorking() const { return flag.get() != 0; }

private:
    juce::Atomic<int32> flag;
    inline bool setWorking (bool status) { return flag.compareAndSetBool (status ? 1 : 0, status ? 0 : 1); }
    friend class WorkThread;
};

class WorkerBase
{
public:
    /** Create a new Worker
        @param thread The WorkThread to use when scheduling
        @param bufsize Size to use for internal response buffers */
    WorkerBase (WorkThread& thread, uint32_t bufsize);
    virtual ~WorkerBase();

    /** Returns true if the worker is currently working */
    inline bool isWorking() const { return flag.isWorking(); }

    /** Schedule work (realtime thread).
        Work will be scheduled, and the thread will call Worker::processRequest
        when the data is queued */
    bool scheduleWork (uint32_t size, const void* data);

    /** Respond from work (worker thread). Call this during processRequest if you
        need to send a response into the realtime thread.
        @see processWorkResponses, @see processResponse */
    bool respondToWork (uint32_t size, const void* data);

    /** Deliver pending responses (realtime thread)
        This must be called regularly from the realtime thread. For each read
        response, Worker::processResponse will be called */
    void processWorkResponses();

    /** Set the internal buffer size for responses */
    void setSize (uint32_t newSize);

protected:
    /** Process work (worker thread) */
    virtual void processRequest (uint32_t size, const void* data) = 0;

    /** Process work responses (realtime thread) */
    virtual void processResponse (uint32_t size, const void* data) = 0;

private:
    WorkThread& owner;
    uint32_t workId; ///< The thread assigned id for this worker
    WorkFlag flag; ///< A flag for when work is being processed

    std::unique_ptr<RingBuffer> responses; ///< responses from work
    juce::HeapBlock<uint8_t> response; ///< buffer to write a response

    bool validateMessage (RingBuffer& ring);

    friend class WorkThread;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WorkerBase)
};

} // namespace element
