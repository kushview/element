
#pragma once

#include "ElementApp.h"

struct Version
{
    Version();
    ~Version();
    
    /** Split a version string into an array of segments */
    inline static StringArray segments (const String& versionString)
    {
        StringArray segments;
        segments.addTokens (versionString, ",.", "");
        segments.trim();
        segments.removeEmptyStrings();
        return segments;
    }
    
    /** Converts version segments into an integer, good for version comparison */
    inline static int asHexInteger (const String& versionString)
    {
        const StringArray segs (segments (versionString));

        int value = (segs[0].getIntValue() << 16)
                  + (segs[1].getIntValue() << 8)
                  + segs[2].getIntValue();
        
        if (segs.size() >= 4)
            value = (value << 8) + segs[3].getIntValue();
        
        return value;
    }
};

class CurrentVersion : private Timer
{
public:
    CurrentVersion();
    ~CurrentVersion();
    
    /** Delay version check. Handles presenting AlertWindow */
    static void checkAfterDelay (const int milliseconds, const bool showUpToDate = false);
    
    /** Returns true if a newer version is available for download */
    bool isNewerVersionAvailable();

private:
    String permalink, version;
    bool hasChecked;
    bool shouldShowUpToDateMessage;
    
    friend class Timer;
    void timerCallback() override;
};
