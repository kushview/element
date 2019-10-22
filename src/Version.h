/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

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

/** Representation of a version number */
struct Version
{
    Version();
    ~Version();
    
    /** Split a version string into an array of segments */
    static StringArray segments (const String& versionString);
    
    /** Converts version segments into an integer, good for version comparison */
    static int asHexInteger (const String& versionString);
};

class CurrentVersion : private Timer,
                       public DeletedAtShutdown,
                       public Thread
{
public:
    CurrentVersion();
    ~CurrentVersion();
    
    /** Delay version check. Handles presenting AlertWindow */
    static void checkAfterDelay (const int milliseconds, const bool showUpToDate = false);
    
    /** Returns true if a newer version is available for download */
    bool isNewerVersionAvailable();
    
    void run() override;
    
private:
    String permalink, version;
    bool hasChecked;
    bool shouldShowUpToDateMessage;
    bool result = false;
    int timeout = 0;
    
    friend class Timer;
    void timerCallback() override;
};
