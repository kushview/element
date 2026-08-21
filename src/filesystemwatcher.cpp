// SPDX-FileCopyrightText: Copyright 2018 by Roland Rabien
// SPDX-License-Identifier: ISC

/*==============================================================================

Copyright 2018 by Roland Rabien
For more information visit www.rabiensoftware.com

==============================================================================*/

#include <algorithm>
#include <ranges>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <climits>
#include <sys/inotify.h>
#include <unistd.h>
#endif

#include <element/juce/events.hpp>
#include "filesystemwatcher.hpp"

#if JUCE_MAC
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <dispatch/dispatch.h>
#endif

namespace element {

//==============================================================================
#if JUCE_MAC

class FileSystemWatcher::Impl
{
public:
    Impl (FileSystemWatcher& o, juce::File f) : owner (o), folder (f)
    {
        const auto* utf8Path = folder.getFullPathName().toRawUTF8();
        const auto pathString = CFStringCreateWithCString (kCFAllocatorDefault, utf8Path, kCFStringEncodingUTF8);

        if (pathString != nullptr)
        {
            const void* values[] = { pathString };
            paths = CFArrayCreate (kCFAllocatorDefault, values, 1, &kCFTypeArrayCallBacks);
            CFRelease (pathString);
        }

        context.version = 0;
        context.info = this;
        context.retain = nullptr;
        context.release = nullptr;
        context.copyDescription = nullptr;

        queue = dispatch_queue_create ("com.gin.filesystemwatcher", DISPATCH_QUEUE_SERIAL);
        stream = FSEventStreamCreate (kCFAllocatorDefault, callback, &context, paths, kFSEventStreamEventIdSinceNow, 0.05, kFSEventStreamCreateFlagNoDefer | kFSEventStreamCreateFlagFileEvents);

        if (stream != nullptr)
        {
            if (queue != nullptr)
                FSEventStreamSetDispatchQueue (stream, queue);

            FSEventStreamStart (stream);
        }
    }

    ~Impl()
    {
        if (stream)
        {
            FSEventStreamStop (stream);
            FSEventStreamSetDispatchQueue (stream, nullptr);
            FSEventStreamInvalidate (stream);
            FSEventStreamRelease (stream);
        }

        if (queue != nullptr)
        {
#if ! (defined(OS_OBJECT_USE_OBJC) && OS_OBJECT_USE_OBJC)
            dispatch_release (queue);
#endif
            queue = nullptr;
        }

        if (paths != nullptr)
        {
            CFRelease (paths);
            paths = nullptr;
        }
    }

    static void callback (ConstFSEventStreamRef streamRef, void* clientCallBackInfo, size_t numEvents, void* eventPaths, const FSEventStreamEventFlags* eventFlags, const FSEventStreamEventId* eventIds)
    {
        juce::ignoreUnused (streamRef, numEvents, eventIds, eventPaths, eventFlags);

        Impl* impl = (Impl*) clientCallBackInfo;

        auto safeOwner = juce::WeakReference<FileSystemWatcher> (&impl->owner);

        juce::MessageManager::callAsync ([safeOwner, f = impl->folder] {
            if (safeOwner)
                safeOwner->folderChanged (f);
        });

        char** files = (char**) eventPaths;

        for (int i = 0; i < int (numEvents); i++)
        {
            char* file = files[i];

            FSEventStreamEventFlags evt = eventFlags[i];

            juce::File path = juce::String::fromUTF8 (file);
            auto event = FileSystemEvent::undefined;

            if (evt & kFSEventStreamEventFlagItemModified)
                event = FileSystemEvent::fileUpdated;
            else if (evt & kFSEventStreamEventFlagItemRemoved)
                event = FileSystemEvent::fileDeleted;
            else if (evt & kFSEventStreamEventFlagItemRenamed)
                event = path.exists() ? FileSystemEvent::fileRenamedNewName : FileSystemEvent::fileRenamedOldName;
            else if (evt & kFSEventStreamEventFlagItemCreated)
                event = FileSystemEvent::fileCreated;

            if (event != FileSystemEvent::undefined)
            {
                juce::MessageManager::callAsync ([safeOwner, path, event] {
                    if (safeOwner)
                        safeOwner->fileChanged (path, event);
                });
            }
        }
    }

    FileSystemWatcher& owner;
    const juce::File folder;

    CFArrayRef paths { nullptr };
    dispatch_queue_t queue { nullptr };
    FSEventStreamRef stream { nullptr };
    struct FSEventStreamContext context
    {
    };
};
#endif

//==============================================================================
#ifdef JUCE_LINUX
#define BUF_LEN (10 * (sizeof (struct inotify_event) + NAME_MAX + 1))

class FileSystemWatcher::Impl final : public juce::Thread,
                                      private juce::AsyncUpdater
{
public:
    struct Event
    {
        Event() = delete;
        Event (const juce::File& f, const FileSystemEvent e) : file (f), fsEvent (e) {}
        Event (Event& other) = default;
        Event (Event&& other) = default;

        juce::File file;
        FileSystemEvent fsEvent = undefined;

        bool operator== (const Event& other) const
        {
            return file == other.file && fsEvent == other.fsEvent;
        }
    };

    Impl (FileSystemWatcher& o, juce::File f)
        : juce::Thread ("FileSystemWatcher::Impl"), owner (o), folder (std::move (f))
    {
        fd = inotify_init();

        wd = inotify_add_watch (fd,
                                folder.getFullPathName().toRawUTF8(),
                                IN_ATTRIB | IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF | IN_MOVED_TO | IN_MOVED_FROM);

        startThread (juce::Thread::Priority::background);
    }

    ~Impl() override
    {
        signalThreadShouldExit();
        inotify_rm_watch (fd, wd);
        close (fd);

        waitForThreadToExit (1000);
    }

    void run() override
    {
        const inotify_event* iNotifyEvent;

        while (! threadShouldExit())
        {
            char buf[BUF_LEN];
            const ssize_t numRead = read (fd, buf, BUF_LEN);

            if (numRead <= 0 || threadShouldExit())
                break;

            for (const char* ptr = buf; ptr < buf + numRead; ptr += sizeof (struct inotify_event) + iNotifyEvent->len)
            {
                iNotifyEvent = reinterpret_cast<const inotify_event*> (ptr);

                FileSystemEvent eventType = undefined;
                if (iNotifyEvent->mask & IN_CREATE)
                    eventType = FileSystemEvent::fileCreated;
                else if (iNotifyEvent->mask & IN_CLOSE_WRITE)
                    eventType = FileSystemEvent::fileUpdated;
                else if (iNotifyEvent->mask & IN_MODIFY)
                    eventType = FileSystemEvent::fileUpdated;
                else if (iNotifyEvent->mask & IN_MOVED_FROM)
                    eventType = FileSystemEvent::fileRenamedOldName;
                else if (iNotifyEvent->mask & IN_MOVED_TO)
                    eventType = FileSystemEvent::fileRenamedNewName;
                else if (iNotifyEvent->mask & IN_DELETE)
                    eventType = FileSystemEvent::fileDeleted;

                if (eventType == FileSystemEvent::undefined)
                {
                    continue;
                }

                juce::ScopedLock sl (lock);
                Event e (folder.getFullPathName() + '/' + iNotifyEvent->name, eventType);
                if (std::ranges::none_of (events, [&] (const auto& event) {
                        return event == e;
                    }))
                {
                    events.add (std::move (e));
                }
            }

            juce::ScopedLock sl (lock);
            if (! events.isEmpty())
            {
                triggerAsyncUpdate();
            }
        }
    }

    void handleAsyncUpdate() override
    {
        juce::ScopedLock sl (lock);

        owner.folderChanged (folder);

        for (const auto& e : events)
        {
            owner.fileChanged (e.file, e.fsEvent);
        }

        events.clear();
    }

    FileSystemWatcher& owner;
    juce::File folder;

    juce::CriticalSection lock;
    juce::Array<Event> events;

    int fd;
    int wd;
};
#endif

//==============================================================================
#ifdef JUCE_WINDOWS
class FileSystemWatcher::Impl : private juce::AsyncUpdater,
                                private juce::Thread
{
public:
    struct Event
    {
        Event() = delete;
        Event (const juce::File& f, FileSystemEvent event) : file (f), fsEvent (event) {}

        juce::File file;
        FileSystemEvent fsEvent = FileSystemEvent::undefined;

        bool operator== (const Event& other) const
        {
            return file == other.file && fsEvent == other.fsEvent;
        }
    };

    Impl (FileSystemWatcher& o, juce::File f)
        : Thread ("FileSystemWatcher::Impl"), owner (o), folder (std::move (f))
    {
        WCHAR path[_MAX_PATH] = { 0 };
        wcsncpy_s (path, folder.getFullPathName().toWideCharPointer(), _MAX_PATH - 1);

        folderHandle = CreateFileW (path, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

        if (folderHandle != INVALID_HANDLE_VALUE)
            startThread (juce::Thread::Priority::background);
    }

    ~Impl() override
    {
        if (isThreadRunning())
        {
            signalThreadShouldExit();

            CancelIoEx (folderHandle, nullptr);

            stopThread (1000);
        }

        if (folderHandle != INVALID_HANDLE_VALUE)
            CloseHandle (folderHandle);
    }

    void run() override
    {
        constexpr DWORD bufferSize = 16 * 1024;
        constexpr DWORD filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME
                                 | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE
                                 | FILE_NOTIFY_CHANGE_CREATION;

        // ReadDirectoryChangesW requires the buffer to be DWORD aligned.
        alignas (DWORD) uint8_t buffer[bufferSize];

        while (! threadShouldExit())
        {
            DWORD bytesOut = 0;
            const BOOL success = ReadDirectoryChangesW (folderHandle, buffer, bufferSize, TRUE, filter, &bytesOut, nullptr, nullptr);

            if (threadShouldExit())
                break;

            if (! success)
            {
                // Too many changes arrived at once and the buffer contents were
                // discarded.  Nothing is known about what changed, so ask the
                // listeners to re-read the whole folder.
                if (GetLastError() == ERROR_NOTIFY_ENUM_DIR)
                {
                    triggerAsyncUpdate();
                    continue;
                }

                // The handle was closed, the io cancelled, or the folder went
                // away.  Retrying would spin, so stop watching.
                break;
            }

            if (bytesOut == 0)
                continue;

            juce::ScopedLock sl (lock);

            const uint8_t* rawData = buffer;
            const uint8_t* const bufferEnd = buffer + juce::jmin (bytesOut, bufferSize);

            for (;;)
            {
                if (rawData + sizeof (FILE_NOTIFY_INFORMATION) > bufferEnd)
                    break;

                const auto* const fni = reinterpret_cast<const FILE_NOTIFY_INFORMATION*> (rawData);
                const auto eventType = eventTypeForAction (fni->Action);

                if (eventType != FileSystemEvent::undefined)
                {
                    Event e { folder.getChildFile (juce::String (fni->FileName, fni->FileNameLength / sizeof (wchar_t))), eventType };

                    if (std::ranges::none_of (events, [&] (const auto& event) {
                            return event == e;
                        }))
                    {
                        events.add (std::move (e));
                    }
                }

                if (fni->NextEntryOffset == 0)
                    break;

                rawData += fni->NextEntryOffset;
            }

            triggerAsyncUpdate();
        }
    }

    /** Maps a FILE_NOTIFY_INFORMATION action to a watcher event.

        Windows defines actions beyond the five handled here (alternate data
        stream and file id notifications, for example).  They carry no file
        event of interest, but the entry they arrived in still has to be
        skipped over.

        @param action  the FILE_NOTIFY_INFORMATION::Action to convert
        @return the matching event, or undefined when the action is not one
                that listeners care about
    */
    static FileSystemEvent eventTypeForAction (DWORD action) noexcept
    {
        switch (action)
        {
            case FILE_ACTION_ADDED:
                return fileCreated;
            case FILE_ACTION_RENAMED_NEW_NAME:
                return fileRenamedNewName;
            case FILE_ACTION_MODIFIED:
                return fileUpdated;
            case FILE_ACTION_REMOVED:
                return fileDeleted;
            case FILE_ACTION_RENAMED_OLD_NAME:
                return fileRenamedOldName;
            default:
                return FileSystemEvent::undefined;
        }
    }

    void handleAsyncUpdate() override
    {
        juce::ScopedLock sl (lock);

        owner.folderChanged (folder);

        for (const auto& e : events)
            owner.fileChanged (e.file, e.fsEvent);

        events.clear();
    }

    FileSystemWatcher& owner;
    const juce::File folder;

    juce::CriticalSection lock;
    juce::Array<Event> events;

    HANDLE folderHandle;
};
#endif

#if defined(JUCE_MAC) || defined(JUCE_WINDOWS) || defined(JUCE_LINUX)
FileSystemWatcher::FileSystemWatcher()
{
}

FileSystemWatcher::~FileSystemWatcher()
{
}

void FileSystemWatcher::addFolder (const juce::File& folder)
{
    // You can only listen to folders that exist
    jassert (folder.isDirectory());

    if (! getWatchedFolders().contains (folder))
        watched.add (new Impl (*this, folder));
}

void FileSystemWatcher::removeFolder (const juce::File& folder)
{
    for (int i = watched.size(); --i >= 0;)
    {
        if (watched[i]->folder == folder)
        {
            watched.remove (i);
            break;
        }
    }
}

void FileSystemWatcher::removeAllFolders()
{
    watched.clear();
}

void FileSystemWatcher::addListener (Listener* newListener)
{
    listeners.add (newListener);
}

void FileSystemWatcher::removeListener (Listener* listener)
{
    listeners.remove (listener);
}

void FileSystemWatcher::folderChanged (const juce::File& folder)
{
    listeners.call (&FileSystemWatcher::Listener::folderChanged, folder);
}

void FileSystemWatcher::fileChanged (const juce::File& file, FileSystemEvent fsEvent)
{
    listeners.call (&FileSystemWatcher::Listener::fileChanged, file, fsEvent);
}

juce::Array<juce::File> FileSystemWatcher::getWatchedFolders()
{
    juce::Array<juce::File> res;

    for (const auto* w : watched)
        res.add (w->folder);

    return res;
}
}

#endif
