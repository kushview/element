// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "appinfo.hpp"
#include <element/datapath.hpp>

namespace element {

/** Aggregation of logs for Element.
    Currently only one log context is supported which is the main application
    log.
 */
class Log : public juce::Logger
{
public:
    Log()
    {
        mainlogger = std::make_unique<FileLogger> (
            getMainLogFile(), "Element (main)");
    }

    ~Log()
    {
        listeners.clear();
        mainlogger.reset();
    }

    class Listener
    {
    public:
        Listener() = default;
        virtual ~Listener() = default;
        virtual void messageLogged (const String& msg) = 0;
    };

    /** Returns the absolute path to the main log file */
    static File getMainLogFile()
    {
        return DataPath::defaultSettingsFile()
            .getParentDirectory()
            .getChildFile ("log/main.log");
    }

    /** Add a listener to receive callbacks */
    void addListener (Listener* listener) { listeners.add (listener); }

    /** Remove a listener */
    void removeListener (Listener* listener) { listeners.remove (listener); }

    /** Returns a copy of the message history */
    StringArray getHistory() const
    {
        ScopedLock sl (lock);
        return history;
    }

    /** Flush message history */
    void flushHistory()
    {
        ScopedLock sl (lock);
        history.clear();
    }

    /** Write a message to the main log.
        
        @param message The message to log.
    */
    void logMessage (const String& message) override
    {
        ScopedLock sl (lock);
        mainlogger->logMessage (message);
        history.add (message);
        if (history.size() > maxLines)
            history.remove (0);

        listeners.call ([&message] (Listener& l) {
            l.messageLogged (message);
        });
    }

private:
    CriticalSection lock;
    std::unique_ptr<FileLogger> mainlogger;
    StringArray history;
    int maxLines = 1024;
    ListenerList<Listener> listeners;
};

} // namespace element
