
#include "Version.h"

#ifndef TEST_CURRENT_VERSION
 #define TEST_CURRENT_VERSION 0
#endif

Version::Version() { }
Version::~Version() { }

StringArray Version::segments (const String& versionString)
{
    StringArray segments;
    segments.addTokens (versionString, ",.", "");
    segments.trim();
    segments.removeEmptyStrings();
    return segments;
}

int Version::asHexInteger (const String& versionString)
{
    const StringArray segs (segments (versionString));
    
    int value = (segs[0].getIntValue() << 16)
              + (segs[1].getIntValue() << 8)
              + segs[2].getIntValue();
    
    if (segs.size() >= 4)
        value = (value << 8) + segs[3].getIntValue();
    
    return value;
}

CurrentVersion::CurrentVersion()
    : Thread("elVersionCheck"),
      version (ProjectInfo::versionString),
      hasChecked (false) { }

CurrentVersion::~CurrentVersion()
{
    if (isThreadRunning())
        stopThread (1000);
}

void CurrentVersion::checkAfterDelay (const int milliseconds, const bool showUpToDate)
{
    auto* cv = new CurrentVersion();
    cv->timeout = milliseconds;
    cv->hasChecked = false;
    cv->shouldShowUpToDateMessage = showUpToDate;
    cv->startThread();
}

bool CurrentVersion::isNewerVersionAvailable()
{
    if (hasChecked)
        return result;
    
   #if TEST_CURRENT_VERSION
    const URL url ("http://kushview.dev/?edd_action=get_version&item_id=15");
   #else
    const URL url ("https://kushview.net/?edd_action=get_version&item_id=20");
   #endif
    
    var data;
    const Result res (JSON::parse (url.readEntireTextStream(), data));
    if (res.failed() || !data.isObject())
        return false;
    
    permalink   = "https://kushview.net/element/download/"; // data["homepage"].toString();
    version     = data["stable_version"].toString();
    
    return Version::asHexInteger(version) > ProjectInfo::versionNumber;
}

void CurrentVersion::run()
{
    hasChecked = false;
    result = isNewerVersionAvailable();
    hasChecked = true;
    startTimer (timeout);
}

void CurrentVersion::timerCallback()
{
    stopTimer();
    
    if (result)
    {
        if (AlertWindow::showOkCancelBox (AlertWindow::NoIcon, "New Version",
                                          "A new version is available: " + version, "Download"))
        {
            URL(permalink).launchInDefaultBrowser();
        }
    }
    else if (shouldShowUpToDateMessage)
    {
        String msg = "Element v";
        msg << ProjectInfo::versionString << " is currently the newest version available.";
        AlertWindow::showMessageBox(AlertWindow::InfoIcon, "You're up-to-date.", msg);
    }
    
    delete this;
}
