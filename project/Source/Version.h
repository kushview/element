
#pragma once

#include "ElementApp.h"

#ifndef TEST_CURRENT_VERSION
 #define TEST_CURRENT_VERSION 0
#endif

struct Version {
    
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
    CurrentVersion() : version(ProjectInfo::versionString), hasChecked (false) { }
    ~CurrentVersion() { }
    
    static void checkAfterDelay (const int milliseconds, const bool showUpToDate = false)
    {
        auto* cv = new CurrentVersion();
        cv->shouldShowUpToDateMessage = showUpToDate;
        cv->startTimer (milliseconds);
    }
    
    bool isNewerVersionAvailable()
    {
       #if TEST_CURRENT_VERSION
        const URL url ("http://kushview.dev/v1/products/element/version.json");
       #else
        const URL url ("https://kushview.net/v1/products/element/version.json");
       #endif
        
        var data;
        const Result res (JSON::parse (url.readEntireTextStream(), data));
        if (res.failed() || !data.isObject())
            return false;
        
        DBG("Running: " << ProjectInfo::projectName << " v" << ProjectInfo::versionString);
        DBG("Latest: " << data["name"].toString() << " v" << data["version"].toString());
        permalink = data["permalink"].toString();
        version = data["version"];
        
        return Version::asHexInteger(data["version"].toString().trim()) > ProjectInfo::versionNumber;
    }

private:
    String permalink, version;
    bool hasChecked;
    bool shouldShowUpToDateMessage;
    friend class Timer;
    void timerCallback() override
    {
        if (hasChecked) return;
        hasChecked = true;
        if (isNewerVersionAvailable())
        {
            if (AlertWindow::showOkCancelBox (AlertWindow::InfoIcon, "New Version", "A new version is available"))
            {
                URL(permalink).launchInDefaultBrowser();
            }
        }
        else if (shouldShowUpToDateMessage)
        {
            String msg = "Element v"; msg << version << " is currently the newest version available.";
            AlertWindow::showMessageBox(AlertWindow::InfoIcon, "You're up-to-date.", msg);
        }
        
        delete this;
    }
};

