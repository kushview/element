/*
    This file is part of Element
    Copyright (C) 2019-2020  Kushview, LLC.  All rights reserved.

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

#include "ElementApp.h"

#ifndef EL_GIT_VERSION
 #define EL_GIT_VERSION ""
#endif

namespace Element {

/** Representation of a version number */
struct Version
{
    Version();
    ~Version();

    /** Returns the version string with git hash appended */    
    inline static String withGitHash()
    {
        String result = ProjectInfo::versionString;
        if (strlen (EL_GIT_VERSION) > 0)
            result << "-" << EL_GIT_VERSION;
        return result;
    }

    /** Split a version string into an array of segments */
    static StringArray segments (const String& versionString);
    
    /** Converts version segments into an integer, good for version comparison */
    static int asHexInteger (const String& versionString);
};

class CurrentVersion : private Timer,
                       private Thread,
                       public DeletedAtShutdown
{
public:
    CurrentVersion();
    ~CurrentVersion();
    
    /** Delay version check. Handles presenting AlertWindow */
    static void checkAfterDelay (const int milliseconds, const bool showUpToDate = false);
    
    /** Returns true if a newer version is available for download */
    bool isNewerVersionAvailable();

    void cancel() { cancelled.set(true); signalThreadShouldExit(); }
    bool isCancelled() const { return cancelled.get(); }

private:
    String permalink, version;
    Atomic<bool> cancelled = false;
    bool hasChecked;
    bool shouldShowUpToDateMessage;
    bool result = false;
    int timeout = 0;
    
    friend class Timer;
    void timerCallback() override;
    void run() override;
};
}
