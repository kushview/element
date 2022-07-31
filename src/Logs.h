/*
    This file is part of Element.
    Copyright (C) 2021 Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

namespace Element {

/** Aggregation of logs for Element.
    Currently only one log context is supported which is the main application
    log.
 */
class Log : public juce::Logger
{
public:
    Log()
    {
        mainlogger.reset (FileLogger::createDefaultAppLogger (
            getSubPath(), "main.log", "Element (main)"));
    }

    ~Log() {}

    class Listener
    {
    public:
        Listener() = default;
        virtual ~Listener() = default;
        virtual void messageLogged (const String& msg) = 0;
    };

    /** Returns the relative path in the system log directory */
    static String getSubPath() { return "Element/log"; }

    /** Returns the absolute path to the log directory */
    static File getTopDir() { return FileLogger::getSystemLogFileFolder().getChildFile (getSubPath()); }

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

} // namespace Element
