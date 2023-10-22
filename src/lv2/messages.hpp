// Copyright 2022 Michael Fisher <mfisher@lvtk.org>
// SPDX-License-Identifier: ISC

#pragma once

#include <element/juce/events.hpp>

#include <atomic>
#include <thread>

#include <lvtk/memory.hpp>
#include <lvtk/spin_lock.hpp>

namespace lvtk {

struct TryLockAndCall
{
    template <typename Fn>
    void operator() (SpinLock& mutex, Fn&& fn)
    {
        if (mutex.try_lock())
        {
            fn();
            mutex.unlock();
        }
    }
};

struct LockAndCall
{
    template <typename Fn>
    void operator() (SpinLock& mutex, Fn&& fn)
    {
        mutex.lock();
        fn();
        mutex.unlock();
    }
};

struct RealtimeReadTrait
{
    using Read = TryLockAndCall;
    using Write = LockAndCall;
};

struct RealtimeWriteTrait
{
    using Read = LockAndCall;
    using Write = TryLockAndCall;
};

struct MessageHeader
{
    uint32_t portIndex;
    uint32_t protocol;
};

template <typename Header>
struct MessageBuffer
{
    virtual ~MessageBuffer() = default;
    virtual void push_message (Header header, uint32_t size, const void* buffer) = 0;
};

template <typename Header, typename LockTraits>
class Messages : public MessageBuffer<Header>
{
public:
    Messages() { data.reserve (initial_size); }

    void push_message (Header header, uint32_t size, const void* buffer) override
    {
        write (mutex, [&] {
            const auto chars = to_chars (FullHeader { header, size });
            const auto charbuf = static_cast<const char*> (buffer);
            data.insert (data.end(), chars.begin(), chars.end());
            data.insert (data.end(), charbuf, charbuf + size);
        });
    }

    template <typename Callback>
    void read_all (Callback&& callback)
    {
        read (mutex, [&] {
            if (data.empty())
                return;

            const auto end = data.data() + data.size();
            for (auto ptr = data.data(); ptr < end;)
            {
                const auto header = read_unaligned<FullHeader> (ptr);
                callback (header.header, header.size, ptr + sizeof (header));
                ptr += sizeof (header) + header.size;
            }

            data.clear();
        });
    }

private:
    using Read = typename LockTraits::Read;
    Read read;

    using Write = typename LockTraits::Write;
    Write write;

    struct FullHeader
    {
        Header header;
        uint32_t size;
    };

    static constexpr auto initial_size = 8192;
    SpinLock mutex;
    std::vector<char> data;
};

//==============================================================================
class LambdaTimer : private juce::Timer
{
public:
    explicit LambdaTimer (std::function<void()> c) : callback (c) {}
    ~LambdaTimer() noexcept override { stopTimer(); }

    using Timer::startTimer;
    using Timer::startTimerHz;
    using Timer::stopTimer;

private:
    void timerCallback() override { callback(); }
    std::function<void()> callback;
};

struct UiEventListener : public MessageBuffer<MessageHeader>
{
    virtual int idle() = 0;
};

struct UiMessageHeader
{
    UiEventListener* listener;
    MessageHeader header;
};

class ProcessorToUi : public MessageBuffer<UiMessageHeader>
{
public:
    ProcessorToUi() { timer.startTimerHz (60); }

    void addUi (UiEventListener& l)
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        activeUis.insert (&l);
    }

    void removeUi (UiEventListener& l)
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        activeUis.erase (&l);
    }

    void push_message (UiMessageHeader header, uint32_t size, const void* buffer) override
    {
        processorToUi.push_message (header, size, buffer);
    }

private:
    Messages<UiMessageHeader, RealtimeWriteTrait> processorToUi;
    std::set<UiEventListener*> activeUis;
    LambdaTimer timer { [this] {
        for (auto* l : activeUis)
            if (l->idle() != 0)
                return;

        processorToUi.read_all ([&] (const UiMessageHeader& header, uint32_t size, const char* data) {
            if (activeUis.find (header.listener) != activeUis.cend())
                header.listener->push_message (header.header, size, data);
        });
    } };
};

} // namespace lvtk
