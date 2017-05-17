
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
    : version (ProjectInfo::versionString),
      hasChecked (false) { }

CurrentVersion::~CurrentVersion() { }

void CurrentVersion::checkAfterDelay (const int milliseconds, const bool showUpToDate)
{
    auto* cv = new CurrentVersion();
    cv->shouldShowUpToDateMessage = showUpToDate;
    cv->startTimer (milliseconds);
}

bool CurrentVersion::isNewerVersionAvailable()
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

void CurrentVersion::timerCallback()
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
        String msg = "Element v";
        msg << ProjectInfo::versionString << " is currently the newest version available.";
        AlertWindow::showMessageBox(AlertWindow::InfoIcon, "You're up-to-date.", msg);
    }
    
    delete this;
}
