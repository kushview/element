// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fcntl.h>

#if JUCE_WINDOWS
#include <io.h>
#else
#include <unistd.h>
#endif

#include <element/juce/events.hpp>

#include "crashlog.hpp"

namespace element {

namespace {

// The log path is captured at install time as a plain buffer. The file is
// opened on demand: open(2) is async-signal-safe, and a descriptor held from
// install time would go stale when juce::FileLogger trims the log by swapping
// in a new file.
constexpr size_t maxPathLength = 4096;
char logPath[maxPathLength] = { 0 };
std::atomic<bool> installed { false };
std::atomic<bool> terminateLogged { false };
std::terminate_handler previousTerminate = nullptr;

constexpr const char* signalPrefix = "\n[element] crash: fatal signal ";
constexpr const char* exceptionPrefix = "\n[element] crash: fatal exception";
constexpr const char* afterTerminate = " (after terminate)";

#if JUCE_WINDOWS
int openAppend()
{
    return logPath[0] != 0 ? _open (logPath, _O_WRONLY | _O_APPEND | _O_CREAT, 0644) : -1;
}
void writeRaw (int fd, const char* text, size_t length)
{
    if (fd >= 0)
        _write (fd, text, (unsigned int) length);
}
void syncFd (int fd)
{
    if (fd >= 0)
        _commit (fd);
}
void closeFd (int fd)
{
    if (fd >= 0)
        _close (fd);
}
#else
int openAppend()
{
    return logPath[0] != 0 ? ::open (logPath, O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC, 0644) : -1;
}
void writeRaw (int fd, const char* text, size_t length)
{
    if (fd < 0)
        return;
    while (length > 0)
    {
        const auto written = ::write (fd, text, length);
        if (written <= 0)
            return;
        text += written;
        length -= (size_t) written;
    }
}
void syncFd (int fd)
{
    if (fd >= 0)
        ::fsync (fd);
}
void closeFd (int fd)
{
    if (fd >= 0)
        ::close (fd);
}
#endif

/** Writes to an open crash log descriptor and to stderr. */
void writeText (int fd, const char* text, size_t length)
{
    writeRaw (fd, text, length);
    writeRaw (2, text, length);
}

void writeText (int fd, const juce::String& text)
{
    writeText (fd, text.toRawUTF8(), text.getNumBytesAsUTF8());
}

juce::String threadDescription()
{
    if (auto* mm = juce::MessageManager::getInstanceWithoutCreating())
        if (mm->isThisTheMessageThread())
            return "message";

    if (auto* thread = juce::Thread::getCurrentThread())
        return thread->getThreadName();

    return "native " + juce::String::toHexString ((juce::pointer_sized_int) juce::Thread::getCurrentThreadId());
}

void onTerminate()
{
    juce::String text;
    text << "\n[element] terminate: " << CrashLog::describeCurrentException()
         << " (thread: " << threadDescription() << ")\n"
         << juce::SystemStats::getStackBacktrace() << "\n";
    const int fd = openAppend();
    writeText (fd, text);
    syncFd (fd);
    closeFd (fd);
    terminateLogged.store (true);

    if (previousTerminate != nullptr)
        previousTerminate();
    std::abort();
}

/** Formats a non-negative integer without allocating. Returns the length. */
size_t formatInt (int value, char* out, size_t capacity)
{
    char digits[16];
    size_t n = 0;
    if (value <= 0)
        digits[n++] = '0';
    while (value > 0 && n < sizeof (digits))
    {
        digits[n++] = (char) ('0' + (value % 10));
        value /= 10;
    }
    size_t written = 0;
    while (n > 0 && written < capacity)
        out[written++] = digits[--n];
    return written;
}

void onCrash (void* info)
{
    const int fd = openAppend();

#if JUCE_WINDOWS
    juce::ignoreUnused (info);
    writeRaw (fd, exceptionPrefix, std::strlen (exceptionPrefix));
    writeRaw (2, exceptionPrefix, std::strlen (exceptionPrefix));
#else
    char number[16];
    const auto length = formatInt ((int) (juce::pointer_sized_int) info, number, sizeof (number));
    writeRaw (fd, signalPrefix, std::strlen (signalPrefix));
    writeRaw (fd, number, length);
    writeRaw (2, signalPrefix, std::strlen (signalPrefix));
    writeRaw (2, number, length);
#endif

    if (terminateLogged.load())
        writeText (fd, afterTerminate, std::strlen (afterTerminate));
    writeText (fd, "\n", 1);
    syncFd (fd);

    // The safe record is on disk. Anything below allocates and is best effort.
    const auto trace = juce::SystemStats::getStackBacktrace();
    writeText (fd, trace);
    writeText (fd, "\n", 1);
    syncFd (fd);
    closeFd (fd);
}

} // namespace

void CrashLog::install (const juce::File& logFile)
{
    if (installed.exchange (true))
        return;

    logFile.getParentDirectory().createDirectory();
    const auto path = logFile.getFullPathName();
    std::memset (logPath, 0, sizeof (logPath));
    std::strncpy (logPath, path.toRawUTF8(), sizeof (logPath) - 1);
    terminateLogged.store (false);
    previousTerminate = std::set_terminate (onTerminate);
    juce::SystemStats::setApplicationCrashHandler (onCrash);
}

void CrashLog::uninstall()
{
    if (! installed.exchange (false))
        return;

    std::set_terminate (previousTerminate);
    previousTerminate = nullptr;

#if ! JUCE_WINDOWS
    // JUCE offers no way to remove its crash handler; restore the defaults for
    // the signals it hooked so a later crash behaves as if we were never here.
    for (const int sig : { SIGFPE, SIGILL, SIGSEGV, SIGBUS, SIGABRT, SIGSYS })
        std::signal (sig, SIG_DFL);
#endif

    std::memset (logPath, 0, sizeof (logPath));
}

bool CrashLog::isInstalled()
{
    return installed.load();
}

juce::String CrashLog::describeCurrentException()
{
    const auto current = std::current_exception();
    if (! current)
        return "no active exception";

    try
    {
        std::rethrow_exception (current);
    } catch (const std::exception& e)
    {
        return juce::String ("std::exception: ") + e.what();
    } catch (const juce::String& s)
    {
        return "juce::String: " + s;
    } catch (const char* s)
    {
        return juce::String ("const char*: ") + (s != nullptr ? s : "");
    } catch (...)
    {
        return "unknown";
    }
}

} // namespace element
